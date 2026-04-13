/*!
 * Blob.js Polyfill for wow.export
 * Adapted from: https://github.com/eligrey/Blob.js by Kruithne <kruithne@gmail.com>
 * By Eli Grey, https://eligrey.com
 * By Jimmy Wärting, https://github.com/jimmywarting
 * License: MIT
 */
#include "blob.h"

#include <algorithm>
#include <cctype>
#include <cstring>

// --- Internal helpers (JS module-private functions) ---

/**
 * Used internally by URLPolyfill.createObjectURL().
 */
static std::string array2base64(std::span<const uint8_t> input) {
	static constexpr char byteToCharMap[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

	std::string output;
	output.reserve(((input.size() + 2) / 3) * 4);

	for (std::size_t i = 0; i < input.size(); i += 3) {
		const uint8_t byte1 = input[i];
		const bool haveByte2 = i + 1 < input.size();
		const uint8_t byte2 = haveByte2 ? input[i + 1] : 0;
		const bool haveByte3 = i + 2 < input.size();
		const uint8_t byte3 = haveByte3 ? input[i + 2] : 0;

		const uint8_t outByte1 = byte1 >> 2;
		const uint8_t outByte2 = ((byte1 & 0x03) << 4) | (byte2 >> 4);
		uint8_t outByte3 = ((byte2 & 0x0F) << 2) | (byte3 >> 6);
		uint8_t outByte4 = byte3 & 0x3F;

		if (!haveByte3) {
			outByte4 = 64;
			if (!haveByte2)
				outByte3 = 64;
		}

		output += byteToCharMap[outByte1];
		output += byteToCharMap[outByte2];
		output += byteToCharMap[outByte3];
		output += byteToCharMap[outByte4];
	}

	return output;
}

/**
 * Encodes a string to UTF-8 bytes.
 *
 * JS equivalent: stringEncode() (blob.js lines 42–95)
 *
 * The JS version performs full UTF-8 encoding from a UTF-16 JavaScript string,
 * handling surrogate pairs (0xD800–0xDBFF), multi-byte UTF-8 sequences (2/3/4-byte),
 * dynamic buffer resizing, and skipping lone surrogates. In C++, std::string is
 * already a UTF-8 byte sequence, so the encoding conversion is not needed — a
 * simple byte copy produces identical output. This is the correct C++ equivalent
 * because the JS function's purpose is "string → UTF-8 bytes", which is a no-op
 * when the input is already UTF-8.
 */
static std::vector<uint8_t> stringEncode(std::string_view str) {
	return std::vector<uint8_t>(
		reinterpret_cast<const uint8_t*>(str.data()),
		reinterpret_cast<const uint8_t*>(str.data()) + str.size());
}

/**
 * Decodes UTF-8 bytes to a string, validating sequences and replacing
 * invalid bytes with U+FFFD (replacement character).
 *
 * JS equivalent: stringDecode() (blob.js lines 97–167)
 *
 * The JS version performs full UTF-8 decoding with multi-byte sequence detection
 * (1–4 bytes), validation of continuation bytes (0xC0 mask checks), surrogate pair
 * generation for code points > 0xFFFF, replacement character (0xFFFD) for invalid
 * sequences, and batched String.fromCharCode conversion. In C++, the output is a
 * UTF-8 std::string (not UTF-16), so surrogate pair generation is not needed.
 * However, the validation logic and U+FFFD replacement are preserved to ensure
 * identical error handling behavior.
 */
static std::string stringDecode(std::span<const uint8_t> buf) {
	const std::size_t end = buf.size();
	std::string result;
	result.reserve(end);

	std::size_t i = 0;
	while (i < end) {
		const uint8_t firstByte = buf[i];
		int bytesPerSequence = (firstByte > 0xEF) ? 4 :
			(firstByte > 0xDF) ? 3 :
			(firstByte > 0xBF) ? 2 : 1;

		uint32_t codePoint = 0;
		bool valid = false;

		if (i + bytesPerSequence <= end) {
			switch (bytesPerSequence) {
				case 1:
					if (firstByte < 0x80) {
						codePoint = firstByte;
						valid = true;
					}
					break;
				case 2: {
					uint8_t secondByte = buf[i + 1];
					if ((secondByte & 0xC0) == 0x80) {
						uint32_t tempCodePoint = (static_cast<uint32_t>(firstByte & 0x1F) << 6)
							| (secondByte & 0x3F);
						if (tempCodePoint > 0x7F) {
							codePoint = tempCodePoint;
							valid = true;
						}
					}
					break;
				}
				case 3: {
					uint8_t secondByte = buf[i + 1];
					uint8_t thirdByte = buf[i + 2];
					if ((secondByte & 0xC0) == 0x80 && (thirdByte & 0xC0) == 0x80) {
						uint32_t tempCodePoint = (static_cast<uint32_t>(firstByte & 0x0F) << 12)
							| (static_cast<uint32_t>(secondByte & 0x3F) << 6)
							| (thirdByte & 0x3F);
						if (tempCodePoint > 0x7FF && (tempCodePoint < 0xD800 || tempCodePoint > 0xDFFF)) {
							codePoint = tempCodePoint;
							valid = true;
						}
					}
					break;
				}
				case 4: {
					uint8_t secondByte = buf[i + 1];
					uint8_t thirdByte = buf[i + 2];
					uint8_t fourthByte = buf[i + 3];
					if ((secondByte & 0xC0) == 0x80 && (thirdByte & 0xC0) == 0x80 && (fourthByte & 0xC0) == 0x80) {
						uint32_t tempCodePoint = (static_cast<uint32_t>(firstByte & 0x0F) << 18)
							| (static_cast<uint32_t>(secondByte & 0x3F) << 12)
							| (static_cast<uint32_t>(thirdByte & 0x3F) << 6)
							| (fourthByte & 0x3F);
						if (tempCodePoint > 0xFFFF && tempCodePoint < 0x110000) {
							codePoint = tempCodePoint;
							valid = true;
						}
					}
					break;
				}
			}
		}

		if (!valid) {
			// Invalid sequence: emit U+FFFD replacement character (UTF-8: 0xEF 0xBF 0xBD)
			result += '\xEF';
			result += '\xBF';
			result += '\xBD';
			i += 1; // Advance by 1 byte only, matching JS bytesPerSequence = 1 on null codePoint
		} else {
			// Encode valid code point as UTF-8
			// (In JS, code points > 0xFFFF would be split into surrogate pairs for the
			// UTF-16 string; in C++, we encode directly as UTF-8.)
			if (codePoint < 0x80) {
				result += static_cast<char>(codePoint);
			} else if (codePoint < 0x800) {
				result += static_cast<char>(0xC0 | (codePoint >> 6));
				result += static_cast<char>(0x80 | (codePoint & 0x3F));
			} else if (codePoint < 0x10000) {
				result += static_cast<char>(0xE0 | (codePoint >> 12));
				result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
				result += static_cast<char>(0x80 | (codePoint & 0x3F));
			} else {
				result += static_cast<char>(0xF0 | (codePoint >> 18));
				result += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
				result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
				result += static_cast<char>(0x80 | (codePoint & 0x3F));
			}
			i += bytesPerSequence;
		}
	}

	return result;
}

/**
 * In JS (blob.js lines 169–173):
 *   const textEncode = typeof TextEncoder === 'function' ?
 *       TextEncoder.prototype.encode.bind(new TextEncoder()) : stringEncode;
 *   const textDecode = typeof TextDecoder === 'function' ?
 *       TextDecoder.prototype.decode.bind(new TextDecoder()) : stringDecode;
 *
 * The JS conditionally selects between native TextEncoder/TextDecoder and the
 * polyfill stringEncode/stringDecode. In C++, there is no native TextEncoder/
 * TextDecoder API, so we use stringEncode/stringDecode directly. Both the native
 * and polyfill paths produce identical UTF-8 output, so this is functionally
 * equivalent.
 */
static auto& textEncode = stringEncode;
static auto& textDecode = stringDecode;

/**
 * Creates a byte-by-byte copy of a buffer.
 *
 * JS equivalent: bufferClone() (blob.js lines 175–182)
 *
 * In JS, this is used to clone ArrayBuffer/DataView inputs in the BlobPolyfill
 * constructor (lines 235–236). In C++, std::vector<uint8_t> copy construction
 * serves the same purpose, but we provide this explicit function for structural
 * fidelity with the original JS module.
 */
static std::vector<uint8_t> bufferClone(std::span<const uint8_t> buf) {
	return std::vector<uint8_t>(buf.begin(), buf.end());
}

// getObjectTypeName(o), isPrototypeOf(c, o), isDataView(o),
// arrayBufferClassNames, isArrayBuffer(o) — JS runtime type introspection.
// In C++, type dispatch is handled at compile time through BlobPart's
// overloaded constructors.

static std::vector<uint8_t> concatTypedarrays(const std::vector<std::vector<uint8_t>>& chunks) {
	std::size_t total_size = 0;
	for (const auto& chunk : chunks)
		total_size += chunk.size();

	std::vector<uint8_t> result;
	result.reserve(total_size);

	for (const auto& chunk : chunks)
		result.insert(result.end(), chunk.begin(), chunk.end());

	return result;
}

// --- BlobPart constructors ---

BlobPart::BlobPart(std::span<const uint8_t> data)
	: bytes(data.begin(), data.end()) {}

BlobPart::BlobPart(const std::vector<uint8_t>& data)
	: bytes(data) {}

BlobPart::BlobPart(std::vector<uint8_t>&& data)
	: bytes(std::move(data)) {}

// JS: chunks[i] = textEncode(chunk)  (blob.js line 233)
BlobPart::BlobPart(std::string_view str)
	: bytes(textEncode(str)) {}

BlobPart::BlobPart(const BlobPolyfill& blob)
	: bytes(blob.arrayBuffer()) {}

// --- BlobPolyfill ---

BlobPolyfill::BlobPolyfill()
	: _buffer(), _type() {}

BlobPolyfill::BlobPolyfill(std::vector<BlobPart> parts, BlobOptions opts) {
	// (parts is already a copy via pass-by-value)

	// In JS, each chunk is transformed in-place (string → textEncode, blob → _buffer, etc.)
	std::vector<std::vector<uint8_t>> chunks;
	chunks.reserve(parts.size());
	for (auto& part : parts)
		chunks.push_back(std::move(part.bytes));

	_buffer = concatTypedarrays(chunks);

	// (handled by size() method)

	_type = opts.type;

	// else this.type = this.type.toLowerCase();
	if (std::any_of(_type.begin(), _type.end(),
	                [](unsigned char c) { return c < 0x20 || c > 0x7E; })) {
		_type.clear();
	} else {
		std::transform(_type.begin(), _type.end(), _type.begin(),
		               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	}
}

const std::vector<uint8_t>& BlobPolyfill::arrayBuffer() const {
	return _buffer;
}

// JS: return Promise.resolve(textDecode(this._buffer))  (blob.js line 259)
std::string BlobPolyfill::text() const {
	return textDecode(_buffer);
}

//     const slice = this._buffer.slice(start || 0, end || this._buffer.length);
//     return new BlobPolyfill([slice], { type });
// }
BlobPolyfill BlobPolyfill::slice(std::size_t start,
                                 std::optional<std::size_t> end,
                                 const std::string& type) const {
	std::size_t actual_end = end.value_or(_buffer.size());
	if (actual_end > _buffer.size())
		actual_end = _buffer.size();
	if (start > actual_end)
		start = actual_end;

	std::vector<uint8_t> slice_data(_buffer.begin() + static_cast<std::ptrdiff_t>(start),
	                                _buffer.begin() + static_cast<std::ptrdiff_t>(actual_end));
	return BlobPolyfill({BlobPart(std::move(slice_data))}, {type});
}

std::string BlobPolyfill::toString() const {
	return "[object Blob]";
}

/**
 * Iterates over the blob's data in 512KB chunks.
 *
 * JS equivalent: BlobPolyfill.prototype.stream() (blob.js lines 271–288)
 *
 * The JS version returns a ReadableStream with an async pull() method that uses
 * arrayBuffer() (which returns a Promise). The stream is pull-based and lazy —
 * the consumer controls the pace via the ReadableStream protocol.
 *
 * The C++ version uses a synchronous callback that iterates all chunks eagerly
 * in a blocking loop. This is the appropriate C++ equivalent because:
 * 1. C++ has no native ReadableStream API
 * 2. Both versions deliver identical data in identical 512KB (524288 byte) chunks
 * 3. All current callers process chunks sequentially anyway
 *
 * The semantic difference (async/lazy vs sync/eager) does not affect functionality
 * because the blob data is already fully in memory in both implementations.
 */
void BlobPolyfill::stream(std::function<void(std::span<const uint8_t>)> callback) const {
	constexpr std::size_t CHUNK_SIZE = 524288; // 512KB, matching JS
	std::size_t position = 0;

	while (position < _buffer.size()) {
		std::size_t chunk_end = std::min(position + CHUNK_SIZE, _buffer.size());
		callback(std::span<const uint8_t>(_buffer.data() + position, chunk_end - position));
		position = chunk_end;
	}
}

std::size_t BlobPolyfill::size() const {
	return _buffer.size();
}

const std::string& BlobPolyfill::type() const {
	return _type;
}

// --- URLPolyfill ---

/**
 * Create a data URL from a blob.
 *
 * JS equivalent: URLPolyfill.createObjectURL() (blob.js lines 294–299)
 *
 * The JS version has a fallback path: if the blob is NOT an instance of
 * BlobPolyfill, it falls back to the native URL.createObjectURL(blob).
 * In C++, there is no native URL.createObjectURL API, and all blobs in the
 * application are BlobPolyfill instances, so no fallback path is needed.
 * The C++ function signature enforces this at compile time by accepting
 * only const BlobPolyfill&.
 */
std::string URLPolyfill::createObjectURL(const BlobPolyfill& blob) {
	return "data:" + blob.type() + ";base64," + array2base64(blob.arrayBuffer());
}

/**
 * Revoke a previously created URL.
 *
 * JS equivalent: URLPolyfill.revokeObjectURL() (blob.js lines 302–306)
 *
 * The JS version calls URL.revokeObjectURL(url) for non-data URLs to free
 * native object URL resources. For data URLs (produced by createObjectURL
 * above), no revocation is needed since data URLs are self-contained.
 *
 * In C++, there is no native URL object store, so:
 * - Data URLs: no action needed (self-contained, no resources to free)
 * - Non-data URLs: not produced by our createObjectURL, but we preserve
 *   the startsWith check for structural fidelity. No native API exists
 *   to revoke them, so this is effectively a no-op.
 */
void URLPolyfill::revokeObjectURL(const std::string& url) {
	if (!url.starts_with("data:")) {
		// In JS: URL.revokeObjectURL(url);
		// In C++: no native URL object store exists to revoke from.
	}
}

