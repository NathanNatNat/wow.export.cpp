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
 * JS: array2base64(input) — base64 encode a byte array.
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
 * JS: stringEncode(string) — converts a JS string (UTF-16) to UTF-8 bytes.
 * In C++, std::string_view is already a UTF-8 byte sequence, so this is
 * a simple byte reinterpretation (no encoding conversion needed).
 *
 * JS: textEncode is either native TextEncoder or stringEncode.
 * In C++, there is only one implementation (strings are already UTF-8).
 */
static std::vector<uint8_t> stringEncode(std::string_view str) {
	return std::vector<uint8_t>(
		reinterpret_cast<const uint8_t*>(str.data()),
		reinterpret_cast<const uint8_t*>(str.data()) + str.size());
}

/**
 * JS: stringDecode(buf) — converts UTF-8 bytes to a JS string (UTF-16).
 * In C++, std::string is already UTF-8, so this is a simple byte copy.
 *
 * JS: textDecode is either native TextDecoder or stringDecode.
 * In C++, there is only one implementation (strings are already UTF-8).
 */
static std::string stringDecode(std::span<const uint8_t> buf) {
	return std::string(reinterpret_cast<const char*>(buf.data()), buf.size());
}

// JS: bufferClone(buf) — clone an ArrayBuffer.
// Not needed in C++: std::vector copy construction handles this.

// JS: getObjectTypeName(o), isPrototypeOf(c, o), isDataView(o),
// arrayBufferClassNames, isArrayBuffer(o) — JS runtime type introspection.
// Not needed in C++: the type system handles this at compile time
// through BlobPart's overloaded constructors.

/**
 * JS: concatTypedarrays(chunks) — concatenate typed arrays into one.
 */
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

// JS: if (typeof chunk === 'string') chunks[i] = textEncode(chunk);
BlobPart::BlobPart(std::string_view str)
	: bytes(stringEncode(str)) {}

// JS: if (chunk instanceof BlobPolyfill) chunks[i] = chunk._buffer;
BlobPart::BlobPart(const BlobPolyfill& blob)
	: bytes(blob.arrayBuffer()) {}

// --- BlobPolyfill ---

BlobPolyfill::BlobPolyfill()
	: _buffer(), _type() {}

BlobPolyfill::BlobPolyfill(std::vector<BlobPart> parts, BlobOptions opts) {
	// JS: chunks = chunks ? chunks.slice() : [];
	// (parts is already a copy via pass-by-value)

	// JS: Collect all chunk buffers, then concatenate via concatTypedarrays.
	// In JS, each chunk is transformed in-place (string → textEncode, blob → _buffer, etc.)
	// In C++, BlobPart constructors handle this transformation.
	std::vector<std::vector<uint8_t>> chunks;
	chunks.reserve(parts.size());
	for (auto& part : parts)
		chunks.push_back(std::move(part.bytes));

	// JS: this._buffer = Uint8Array ? concatTypedarrays(chunks) : [].concat.apply([], chunks);
	_buffer = concatTypedarrays(chunks);

	// JS: this.size = this._buffer.length;
	// (handled by size() method)

	// JS: this.type = opts.type || '';
	_type = opts.type;

	// JS: if (/[^\u0020-\u007E]/.test(this.type)) this.type = '';
	// else this.type = this.type.toLowerCase();
	if (std::any_of(_type.begin(), _type.end(),
	                [](unsigned char c) { return c < 0x20 || c > 0x7E; })) {
		_type.clear();
	} else {
		std::transform(_type.begin(), _type.end(), _type.begin(),
		               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	}
}

// JS: arrayBuffer() { return Promise.resolve(this._buffer.buffer || this._buffer); }
const std::vector<uint8_t>& BlobPolyfill::arrayBuffer() const {
	return _buffer;
}

// JS: text() { return Promise.resolve(textDecode(this._buffer)); }
std::string BlobPolyfill::text() const {
	return stringDecode(_buffer);
}

// JS: slice(start, end, type) {
//     const slice = this._buffer.slice(start || 0, end || this._buffer.length);
//     return new BlobPolyfill([slice], { type });
// }
BlobPolyfill BlobPolyfill::slice(std::size_t start,
                                 std::optional<std::size_t> end,
                                 const std::string& type) const {
	// JS: end || this._buffer.length — if end is 0 or undefined, use full length
	std::size_t actual_end = end.value_or(_buffer.size());
	if (actual_end > _buffer.size())
		actual_end = _buffer.size();
	if (start > actual_end)
		start = actual_end;

	std::vector<uint8_t> slice_data(_buffer.begin() + static_cast<std::ptrdiff_t>(start),
	                                _buffer.begin() + static_cast<std::ptrdiff_t>(actual_end));
	return BlobPolyfill({BlobPart(std::move(slice_data))}, {type});
}

// JS: toString() { return '[object Blob]'; }
std::string BlobPolyfill::toString() const {
	return "[object Blob]";
}

// JS: stream() — returns a ReadableStream that yields 512KB chunks.
// C++ uses a callback instead of ReadableStream.
void BlobPolyfill::stream(std::function<void(std::span<const uint8_t>)> callback) const {
	constexpr std::size_t CHUNK_SIZE = 524288; // 512KB, matching JS
	std::size_t position = 0;

	while (position < _buffer.size()) {
		std::size_t chunk_end = std::min(position + CHUNK_SIZE, _buffer.size());
		callback(std::span<const uint8_t>(_buffer.data() + position, chunk_end - position));
		position = chunk_end;
	}
}

// JS: get size() equivalent
std::size_t BlobPolyfill::size() const {
	return _buffer.size();
}

// JS: get type() equivalent
const std::string& BlobPolyfill::type() const {
	return _type;
}

// --- URLPolyfill ---

// JS: static createObjectURL(blob) {
//     if (blob instanceof BlobPolyfill)
//         return 'data:' + blob.type + ';base64,' + array2base64(blob._buffer);
//     return URL.createObjectURL(blob); // fallback to native
// }
// In C++, all blobs are BlobPolyfill, so no native fallback is needed.
std::string URLPolyfill::createObjectURL(const BlobPolyfill& blob) {
	return "data:" + blob.type() + ";base64," + array2base64(blob.arrayBuffer());
}

// JS: static revokeObjectURL(url) {
//     if (!url.startsWith('data:'))
//         URL.revokeObjectURL(url); // only revoke non-data URLs
// }
// In C++, createObjectURL always produces data URLs, so this is
// effectively a no-op. We preserve the startsWith check for fidelity.
void URLPolyfill::revokeObjectURL(const std::string& url) {
	if (!url.starts_with("data:")) {
		// In JS: URL.revokeObjectURL(url);
		// In C++: no native URL system exists. No-op.
	}
}

