/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
License: MIT
 */

#include "buffer.h"
#include "crc32.h"
#include "blob.h"

#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <array>
#include <format>

#include <zlib.h>

#include <openssl/evp.h>

#include <nlohmann/json.hpp>

#include <webp/encode.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif
#include <miniaudio.h>

// stb_image_write for PNG encoding in fromPixelData
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include <stb_image_write.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace {


template<size_t N>
uint64_t read_uint_le(const uint8_t* p) {
uint64_t val = 0;
for (size_t i = 0; i < N; i++)
val |= static_cast<uint64_t>(p[i]) << (i * 8);
return val;
}

template<size_t N>
uint64_t read_uint_be(const uint8_t* p) {
uint64_t val = 0;
for (size_t i = 0; i < N; i++)
val |= static_cast<uint64_t>(p[i]) << ((N - 1 - i) * 8);
return val;
}

template<size_t N>
int64_t sign_extend(uint64_t val) {
constexpr uint64_t sign_bit = 1ULL << (N * 8 - 1);
constexpr uint64_t mask = []() constexpr -> uint64_t {
    if constexpr (N < 8) return (1ULL << (N * 8)) - 1;
    else return ~0ULL;
}();
val &= mask;
if (val & sign_bit)
val |= ~mask;
return static_cast<int64_t>(val);
}

template<size_t N, bool LE>
uint64_t read_uint(const uint8_t* p) {
if constexpr (LE) return read_uint_le<N>(p);
else return read_uint_be<N>(p);
}

template<size_t N>
void write_uint_le(uint8_t* p, uint64_t val) {
for (size_t i = 0; i < N; i++)
p[i] = static_cast<uint8_t>(val >> (i * 8));
}

template<size_t N>
void write_uint_be(uint8_t* p, uint64_t val) {
for (size_t i = 0; i < N; i++)
p[i] = static_cast<uint8_t>(val >> ((N - 1 - i) * 8));
}

template<size_t N, bool LE>
void write_uint(uint8_t* p, uint64_t val) {
if constexpr (LE) write_uint_le<N>(p, val);
else write_uint_be<N>(p, val);
}

// Variable-length read helpers (runtime byte length)
uint64_t read_var_uint_le(const uint8_t* p, size_t n) {
uint64_t val = 0;
for (size_t i = 0; i < n; i++)
val |= static_cast<uint64_t>(p[i]) << (i * 8);
return val;
}

uint64_t read_var_uint_be(const uint8_t* p, size_t n) {
uint64_t val = 0;
for (size_t i = 0; i < n; i++)
val |= static_cast<uint64_t>(p[i]) << ((n - 1 - i) * 8);
return val;
}

int64_t sign_extend_var(uint64_t val, size_t n) {
if (n >= 8) return static_cast<int64_t>(val);
uint64_t sign_bit = 1ULL << (n * 8 - 1);
uint64_t mask = (1ULL << (n * 8)) - 1;
val &= mask;
if (val & sign_bit)
val |= ~mask;
return static_cast<int64_t>(val);
}


constexpr char base64_chars[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const uint8_t* data, size_t len) {
std::string result;
result.reserve((len + 2) / 3 * 4);

for (size_t i = 0; i < len; i += 3) {
uint32_t n = static_cast<uint32_t>(data[i]) << 16;
if (i + 1 < len) n |= static_cast<uint32_t>(data[i + 1]) << 8;
if (i + 2 < len) n |= static_cast<uint32_t>(data[i + 2]);

result += base64_chars[(n >> 18) & 0x3f];
result += base64_chars[(n >> 12) & 0x3f];
result += (i + 1 < len) ? base64_chars[(n >> 6) & 0x3f] : '=';
result += (i + 2 < len) ? base64_chars[n & 0x3f] : '=';
}

return result;
}

std::vector<uint8_t> base64_decode(std::string_view str) {
static const auto& get_table = []() -> const std::array<uint8_t, 256>& {
static const auto table = []() {
std::array<uint8_t, 256> t{};
t.fill(255);
for (uint8_t i = 0; i < 64; i++)
t[static_cast<uint8_t>(base64_chars[i])] = i;
return t;
}();
return table;
};
const auto& table = get_table();

std::vector<uint8_t> result;
result.reserve(str.size() * 3 / 4);

uint32_t val = 0;
int bits = 0;
for (char c : str) {
if (c == '=' || c == '\n' || c == '\r') continue;
uint8_t d = table[static_cast<uint8_t>(c)];
if (d == 255) continue;
val = (val << 6) | d;
bits += 6;
if (bits >= 8) {
bits -= 8;
result.push_back(static_cast<uint8_t>((val >> bits) & 0xff));
}
}

return result;
}


std::string bytes_to_hex_string(const uint8_t* data, size_t length) {
	constexpr char hex_chars[] = "0123456789abcdef";
	std::string out;
	out.reserve(length * 2);
	for (size_t i = 0; i < length; ++i) {
		const uint8_t v = data[i];
		out.push_back(hex_chars[v >> 4]);
		out.push_back(hex_chars[v & 0x0F]);
	}
	return out;
}

void append_utf8_from_codepoint(std::string& out, uint32_t cp) {
	if (cp < 0x80) {
		out.push_back(static_cast<char>(cp));
	} else if (cp < 0x800) {
		out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
		out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
	} else if (cp < 0x10000) {
		out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
		out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
	} else {
		out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
		out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
	}
}

std::string decode_utf16le_to_utf8(const uint8_t* data, size_t length) {
	std::string out;
	out.reserve(length);
	size_t i = 0;
	while (i + 1 < length) {
		const uint16_t w1 = static_cast<uint16_t>(data[i] | (static_cast<uint16_t>(data[i + 1]) << 8));
		i += 2;

		if (w1 >= 0xD800 && w1 <= 0xDBFF && i + 1 < length) {
			const uint16_t w2 = static_cast<uint16_t>(data[i] | (static_cast<uint16_t>(data[i + 1]) << 8));
			if (w2 >= 0xDC00 && w2 <= 0xDFFF) {
				i += 2;
				const uint32_t cp = 0x10000 + (((w1 - 0xD800) << 10) | (w2 - 0xDC00));
				append_utf8_from_codepoint(out, cp);
				continue;
			}
		}

		append_utf8_from_codepoint(out, w1);
	}
	return out;
}



static std::vector<uint8_t> evp_hash(const uint8_t* data, size_t len,
                                      const EVP_MD* md) {
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
	if (!ctx)
		throw std::runtime_error("EVP_MD_CTX_new failed");
	if (EVP_DigestInit_ex(ctx, md, nullptr) != 1 ||
	    EVP_DigestUpdate(ctx, data, len) != 1) {
		EVP_MD_CTX_free(ctx);
		throw std::runtime_error("Hash computation failed");
	}
	std::vector<uint8_t> digest(EVP_MD_size(md));
	unsigned int digest_len = 0;
	if (EVP_DigestFinal_ex(ctx, digest.data(), &digest_len) != 1) {
		EVP_MD_CTX_free(ctx);
		throw std::runtime_error("Hash finalization failed");
	}
	EVP_MD_CTX_free(ctx);
	digest.resize(digest_len);
	return digest;
}

static const EVP_MD* hash_name_to_md(std::string_view name) {
	if (name == "md5")    return EVP_md5();
	if (name == "sha1")   return EVP_sha1();
	if (name == "sha224") return EVP_sha224();
	if (name == "sha256") return EVP_sha256();
	if (name == "sha384") return EVP_sha384();
	if (name == "sha512") return EVP_sha512();
	return nullptr;
}

static std::string digest_to_hex(const std::vector<uint8_t>& digest) {
	constexpr char hex_chars[] = "0123456789abcdef";
	std::string result;
	result.reserve(digest.size() * 2);
	for (uint8_t b : digest) {
		result += hex_chars[b >> 4];
		result += hex_chars[b & 0xf];
	}
	return result;
}



std::vector<uint8_t> zlib_inflate(const uint8_t* data, size_t len) {
z_stream strm{};
if (inflateInit(&strm) != Z_OK)
throw std::runtime_error("zlib inflateInit failed");

strm.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data));
strm.avail_in = static_cast<uInt>(len);

std::vector<uint8_t> result;
uint8_t chunk[16384];

int ret;
do {
strm.next_out = chunk;
strm.avail_out = sizeof(chunk);
ret = inflate(&strm, Z_NO_FLUSH);
if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
inflateEnd(&strm);
throw std::runtime_error("zlib inflate failed");
}
result.insert(result.end(), chunk, chunk + sizeof(chunk) - strm.avail_out);
} while (ret != Z_STREAM_END);

inflateEnd(&strm);
return result;
}

std::vector<uint8_t> zlib_deflate(const uint8_t* data, size_t len) {
z_stream strm{};
if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK)
throw std::runtime_error("zlib deflateInit failed");

strm.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data));
strm.avail_in = static_cast<uInt>(len);

std::vector<uint8_t> result;
uint8_t chunk[16384];

int ret;
do {
strm.next_out = chunk;
strm.avail_out = sizeof(chunk);
ret = deflate(&strm, Z_FINISH);
if (ret == Z_STREAM_ERROR) {
deflateEnd(&strm);
throw std::runtime_error("zlib deflate failed");
}
result.insert(result.end(), chunk, chunk + sizeof(chunk) - strm.avail_out);
} while (ret != Z_STREAM_END);

deflateEnd(&strm);
return result;
}

} // anonymous namespace

// =====================================================================
// Macros for repetitive fixed-width read/write definitions
// =====================================================================

#define IMPL_READ_U(RetType, Name, N, LE) \
RetType BufferWrapper::Name() { \
_checkBounds(N); \
auto v = static_cast<RetType>(read_uint<N, LE>(_buf.data() + _ofs)); \
_ofs += N; \
return v; \
} \
std::vector<RetType> BufferWrapper::Name(size_t count) { \
_checkBounds(N * count); \
std::vector<RetType> values(count); \
for (size_t i = 0; i < count; i++) { \
values[i] = static_cast<RetType>(read_uint<N, LE>(_buf.data() + _ofs)); \
_ofs += N; \
} \
return values; \
}

#define IMPL_READ_S(RetType, Name, N, LE) \
RetType BufferWrapper::Name() { \
_checkBounds(N); \
auto v = static_cast<RetType>(sign_extend<N>(read_uint<N, LE>(_buf.data() + _ofs))); \
_ofs += N; \
return v; \
} \
std::vector<RetType> BufferWrapper::Name(size_t count) { \
_checkBounds(N * count); \
std::vector<RetType> values(count); \
for (size_t i = 0; i < count; i++) { \
values[i] = static_cast<RetType>(sign_extend<N>(read_uint<N, LE>(_buf.data() + _ofs))); \
_ofs += N; \
} \
return values; \
}

#define IMPL_WRITE_INT(ParamType, Name, N, LE) \
void BufferWrapper::Name(ParamType value) { \
_checkBounds(N); \
write_uint<N, LE>(_buf.data() + _ofs, static_cast<uint64_t>(value)); \
_ofs += N; \
}

// =====================================================================
// Static factory methods
// =====================================================================

BufferWrapper BufferWrapper::alloc(size_t length, bool secure) {
std::vector<uint8_t> buf(length, 0);
return BufferWrapper(std::move(buf));
}

BufferWrapper BufferWrapper::from(std::span<const uint8_t> source) {
return BufferWrapper(std::vector<uint8_t>(source.begin(), source.end()));
}

BufferWrapper BufferWrapper::from(std::vector<uint8_t>&& source) {
return BufferWrapper(std::move(source));
}

BufferWrapper BufferWrapper::fromBase64(std::string_view source) {
return BufferWrapper(base64_decode(source));
}

BufferWrapper BufferWrapper::concat(const std::vector<BufferWrapper>& buffers) {
size_t total = 0;
for (const auto& b : buffers)
total += b.byteLength();

std::vector<uint8_t> combined;
combined.reserve(total);
for (const auto& b : buffers)
combined.insert(combined.end(), b._buf.begin(), b._buf.end());

return BufferWrapper(std::move(combined));
}

BufferWrapper BufferWrapper::fromCanvas(const uint8_t* rgba, int width, int height,
                                        std::string_view mimeType, int quality) {
if (mimeType == "image/webp" && quality == 100) {
// Lossless WebP encoding — matches JS webp.encode(imageData, { lossless: true })
uint8_t* output = nullptr;
size_t outputSize = WebPEncodeLosslessRGBA(rgba, width, height, width * 4, &output);
if (outputSize == 0 || output == nullptr)
throw std::runtime_error("fromCanvas: WebP lossless encoding failed");

std::vector<uint8_t> buf(output, output + outputSize);
WebPFree(output);
return BufferWrapper(std::move(buf));
}

if (mimeType == "image/webp") {
// Lossy WebP encoding
uint8_t* output = nullptr;
size_t outputSize = WebPEncodeRGBA(rgba, width, height, width * 4,
                                    static_cast<float>(quality), &output);
if (outputSize == 0 || output == nullptr)
throw std::runtime_error("fromCanvas: WebP lossy encoding failed");

std::vector<uint8_t> buf(output, output + outputSize);
WebPFree(output);
return BufferWrapper(std::move(buf));
}

if (mimeType == "image/png") {
// PNG encoding via stb_image_write to memory
std::vector<uint8_t> pngBuf;
auto writeFunc = [](void* context, void* data, int size) {
auto* vec = static_cast<std::vector<uint8_t>*>(context);
auto* bytes = static_cast<const uint8_t*>(data);
vec->insert(vec->end(), bytes, bytes + size);
};
int ret = stbi_write_png_to_func(writeFunc, &pngBuf, width, height, 4, rgba, width * 4);
if (ret == 0)
throw std::runtime_error("fromCanvas: PNG encoding failed");

return BufferWrapper(std::move(pngBuf));
}

if (mimeType == "image/jpeg" || mimeType == "image/jpg") {
	std::vector<uint8_t> jpgBuf;
	auto writeFunc = [](void* context, void* data, int size) {
		auto* vec = static_cast<std::vector<uint8_t>*>(context);
		auto* bytes = static_cast<const uint8_t*>(data);
		vec->insert(vec->end(), bytes, bytes + size);
	};
	int ret = stbi_write_jpg_to_func(writeFunc, &jpgBuf, width, height, 4, rgba, quality);
	if (ret == 0)
		throw std::runtime_error("fromCanvas: JPEG encoding failed");

	return BufferWrapper(std::move(jpgBuf));
}

throw std::runtime_error("fromCanvas: unsupported mimeType '" + std::string(mimeType) + "'");
}

BufferWrapper BufferWrapper::fromPixelData(const uint8_t* rgba, int width, int height,
                                           std::string_view mimeType, int quality) {
	return fromCanvas(rgba, width, height, mimeType, quality);
}

BufferWrapper BufferWrapper::readFile(const std::filesystem::path& file) {
std::ifstream ifs(file, std::ios::binary | std::ios::ate);
if (!ifs)
throw std::runtime_error("Failed to open file: " + file.string());

auto size = ifs.tellg();
ifs.seekg(0, std::ios::beg);

std::vector<uint8_t> buf(static_cast<size_t>(size));
if (!ifs.read(reinterpret_cast<char*>(buf.data()), size))
throw std::runtime_error("Failed to read file: " + file.string());

return BufferWrapper(std::move(buf));
}

BufferWrapper BufferWrapper::fromMmap(void* mmapData, size_t size) {
auto* src = static_cast<const uint8_t*>(mmapData);
BufferWrapper wrapper(std::vector<uint8_t>(src, src + size));
wrapper._mmap = mmapData;
wrapper._mmapSize = size;
return wrapper;
}

// =====================================================================
// Constructors / destructor
// =====================================================================

BufferWrapper::BufferWrapper() : _ofs(0) {}

BufferWrapper::BufferWrapper(std::vector<uint8_t> buf)
: _ofs(0), _buf(std::move(buf)) {}

BufferWrapper::BufferWrapper(std::vector<uint8_t> buf, size_t offset)
: _ofs(offset), _buf(std::move(buf)) {}

BufferWrapper::BufferWrapper(BufferWrapper&& other) noexcept
: _ofs(other._ofs), _buf(std::move(other._buf)), _mmap(other._mmap), _mmapSize(other._mmapSize), dataURL(std::move(other.dataURL)) {
other._ofs = 0;
other._mmap = nullptr;
other._mmapSize = 0;
}

BufferWrapper& BufferWrapper::operator=(BufferWrapper&& other) noexcept {
if (this != &other) {
_ofs = other._ofs;
_buf = std::move(other._buf);
_mmap = other._mmap;
_mmapSize = other._mmapSize;
dataURL = std::move(other.dataURL);
other._ofs = 0;
other._mmap = nullptr;
other._mmapSize = 0;
}
return *this;
}

BufferWrapper::~BufferWrapper() = default;

// =====================================================================
// Property getters
// =====================================================================

size_t BufferWrapper::byteLength() const {
return _buf.size();
}

size_t BufferWrapper::remainingBytes() const {
return _buf.size() - _ofs;
}

size_t BufferWrapper::offset() const {
return _ofs;
}

const std::vector<uint8_t>& BufferWrapper::raw() const {
return _buf;
}

std::vector<uint8_t>& BufferWrapper::raw() {
return _buf;
}

const uint8_t* BufferWrapper::internalArrayBuffer() const {
return _buf.data();
}

// =====================================================================
// Position methods
// =====================================================================

void BufferWrapper::seek(int64_t ofs) {
int64_t pos = ofs < 0 ? static_cast<int64_t>(byteLength()) + ofs : ofs;
if (pos < 0 || static_cast<size_t>(pos) > byteLength())
throw std::runtime_error("seek() offset out of bounds " + std::to_string(ofs) +
" -> " + std::to_string(pos) + " ! " + std::to_string(byteLength()));

_ofs = static_cast<size_t>(pos);
}

void BufferWrapper::move(int64_t ofs) {
int64_t pos = static_cast<int64_t>(offset()) + ofs;
if (pos < 0 || static_cast<size_t>(pos) > byteLength())
throw std::runtime_error("move() offset out of bounds " + std::to_string(ofs) +
" -> " + std::to_string(pos) + " ! " + std::to_string(byteLength()));

_ofs = static_cast<size_t>(pos);
}

// =====================================================================
// Variable-length integer reads
// =====================================================================

int64_t BufferWrapper::readIntLE(size_t byteLength) {
_checkBounds(byteLength);
uint64_t val = read_var_uint_le(_buf.data() + _ofs, byteLength);
_ofs += byteLength;
return sign_extend_var(val, byteLength);
}

std::vector<int64_t> BufferWrapper::readIntLE(size_t byteLength, size_t count) {
_checkBounds(byteLength * count);
std::vector<int64_t> values(count);
for (size_t i = 0; i < count; i++) {
uint64_t val = read_var_uint_le(_buf.data() + _ofs, byteLength);
_ofs += byteLength;
values[i] = sign_extend_var(val, byteLength);
}
return values;
}

uint64_t BufferWrapper::readUIntLE(size_t byteLength) {
_checkBounds(byteLength);
uint64_t val = read_var_uint_le(_buf.data() + _ofs, byteLength);
_ofs += byteLength;
return val;
}

std::vector<uint64_t> BufferWrapper::readUIntLE(size_t byteLength, size_t count) {
_checkBounds(byteLength * count);
std::vector<uint64_t> values(count);
for (size_t i = 0; i < count; i++) {
values[i] = read_var_uint_le(_buf.data() + _ofs, byteLength);
_ofs += byteLength;
}
return values;
}

int64_t BufferWrapper::readIntBE(size_t byteLength) {
_checkBounds(byteLength);
uint64_t val = read_var_uint_be(_buf.data() + _ofs, byteLength);
_ofs += byteLength;
return sign_extend_var(val, byteLength);
}

std::vector<int64_t> BufferWrapper::readIntBE(size_t byteLength, size_t count) {
_checkBounds(byteLength * count);
std::vector<int64_t> values(count);
for (size_t i = 0; i < count; i++) {
uint64_t val = read_var_uint_be(_buf.data() + _ofs, byteLength);
_ofs += byteLength;
values[i] = sign_extend_var(val, byteLength);
}
return values;
}

uint64_t BufferWrapper::readUIntBE(size_t byteLength) {
_checkBounds(byteLength);
uint64_t val = read_var_uint_be(_buf.data() + _ofs, byteLength);
_ofs += byteLength;
return val;
}

std::vector<uint64_t> BufferWrapper::readUIntBE(size_t byteLength, size_t count) {
_checkBounds(byteLength * count);
std::vector<uint64_t> values(count);
for (size_t i = 0; i < count; i++) {
values[i] = read_var_uint_be(_buf.data() + _ofs, byteLength);
_ofs += byteLength;
}
return values;
}

// =====================================================================
// Fixed-width integer reads — 8-bit
// =====================================================================

IMPL_READ_S(int8_t,  readInt8,  1, true)
IMPL_READ_U(uint8_t, readUInt8, 1, true)

// =====================================================================
// Fixed-width integer reads — 16-bit
// =====================================================================

IMPL_READ_S(int16_t,  readInt16LE,  2, true)
IMPL_READ_U(uint16_t, readUInt16LE, 2, true)
IMPL_READ_S(int16_t,  readInt16BE,  2, false)
IMPL_READ_U(uint16_t, readUInt16BE, 2, false)

// =====================================================================
// Fixed-width integer reads — 24-bit
// =====================================================================

IMPL_READ_S(int32_t,  readInt24LE,  3, true)
IMPL_READ_U(uint32_t, readUInt24LE, 3, true)
IMPL_READ_S(int32_t,  readInt24BE,  3, false)
IMPL_READ_U(uint32_t, readUInt24BE, 3, false)

// =====================================================================
// Fixed-width integer reads — 32-bit
// =====================================================================

IMPL_READ_S(int32_t,  readInt32LE,  4, true)
IMPL_READ_U(uint32_t, readUInt32LE, 4, true)
IMPL_READ_S(int32_t,  readInt32BE,  4, false)
IMPL_READ_U(uint32_t, readUInt32BE, 4, false)

// =====================================================================
// Fixed-width integer reads — 40-bit
// =====================================================================

IMPL_READ_S(int64_t,  readInt40LE,  5, true)
IMPL_READ_U(uint64_t, readUInt40LE, 5, true)
IMPL_READ_S(int64_t,  readInt40BE,  5, false)
IMPL_READ_U(uint64_t, readUInt40BE, 5, false)

// =====================================================================
// Fixed-width integer reads — 48-bit
// =====================================================================

IMPL_READ_S(int64_t,  readInt48LE,  6, true)
IMPL_READ_U(uint64_t, readUInt48LE, 6, true)
IMPL_READ_S(int64_t,  readInt48BE,  6, false)
IMPL_READ_U(uint64_t, readUInt48BE, 6, false)

// =====================================================================
// Fixed-width integer reads — 64-bit
// =====================================================================

IMPL_READ_S(int64_t,  readInt64LE,  8, true)
IMPL_READ_U(uint64_t, readUInt64LE, 8, true)
IMPL_READ_S(int64_t,  readInt64BE,  8, false)
IMPL_READ_U(uint64_t, readUInt64BE, 8, false)

// =====================================================================
// Float / double reads
// =====================================================================

float BufferWrapper::readFloatLE() {
_checkBounds(4);
uint32_t bits = static_cast<uint32_t>(read_uint_le<4>(_buf.data() + _ofs));
_ofs += 4;
float f;
std::memcpy(&f, &bits, sizeof(f));
return f;
}

std::vector<float> BufferWrapper::readFloatLE(size_t count) {
_checkBounds(4 * count);
std::vector<float> values(count);
for (size_t i = 0; i < count; i++) {
uint32_t bits = static_cast<uint32_t>(read_uint_le<4>(_buf.data() + _ofs));
_ofs += 4;
std::memcpy(&values[i], &bits, sizeof(float));
}
return values;
}

float BufferWrapper::readFloatBE() {
_checkBounds(4);
uint32_t bits = static_cast<uint32_t>(read_uint_be<4>(_buf.data() + _ofs));
_ofs += 4;
float f;
std::memcpy(&f, &bits, sizeof(f));
return f;
}

std::vector<float> BufferWrapper::readFloatBE(size_t count) {
_checkBounds(4 * count);
std::vector<float> values(count);
for (size_t i = 0; i < count; i++) {
uint32_t bits = static_cast<uint32_t>(read_uint_be<4>(_buf.data() + _ofs));
_ofs += 4;
std::memcpy(&values[i], &bits, sizeof(float));
}
return values;
}

double BufferWrapper::readDoubleLE() {
_checkBounds(8);
uint64_t bits = read_uint_le<8>(_buf.data() + _ofs);
_ofs += 8;
double d;
std::memcpy(&d, &bits, sizeof(d));
return d;
}

std::vector<double> BufferWrapper::readDoubleLE(size_t count) {
_checkBounds(8 * count);
std::vector<double> values(count);
for (size_t i = 0; i < count; i++) {
uint64_t bits = read_uint_le<8>(_buf.data() + _ofs);
_ofs += 8;
std::memcpy(&values[i], &bits, sizeof(double));
}
return values;
}

double BufferWrapper::readDoubleBE() {
_checkBounds(8);
uint64_t bits = read_uint_be<8>(_buf.data() + _ofs);
_ofs += 8;
double d;
std::memcpy(&d, &bits, sizeof(d));
return d;
}

std::vector<double> BufferWrapper::readDoubleBE(size_t count) {
_checkBounds(8 * count);
std::vector<double> values(count);
for (size_t i = 0; i < count; i++) {
uint64_t bits = read_uint_be<8>(_buf.data() + _ofs);
_ofs += 8;
std::memcpy(&values[i], &bits, sizeof(double));
}
return values;
}

// =====================================================================
// String / buffer reads
// =====================================================================

std::string BufferWrapper::readHexString(size_t length) {
_checkBounds(length);
constexpr char hex_chars[] = "0123456789abcdef";
std::string hex;
hex.reserve(length * 2);
for (size_t i = 0; i < length; i++) {
uint8_t byte = _buf[_ofs + i];
hex += hex_chars[byte >> 4];
hex += hex_chars[byte & 0x0f];
}
_ofs += length;
return hex;
}

BufferWrapper BufferWrapper::readBuffer() {
return readBuffer(remainingBytes(), false);
}

BufferWrapper BufferWrapper::readBuffer(size_t length, bool doInflate) {
_checkBounds(length);

std::vector<uint8_t> buf(_buf.begin() + _ofs, _buf.begin() + _ofs + length);
_ofs += length;

if (doInflate)
buf = zlib_inflate(buf.data(), buf.size());

return BufferWrapper(std::move(buf));
}

std::variant<BufferWrapper, std::vector<uint8_t>> BufferWrapper::readBuffer(size_t length, bool wrap, bool inflate) {
	if (wrap)
		return readBuffer(length, inflate);
	return readBufferRaw(length, inflate);
}

std::vector<uint8_t> BufferWrapper::readBufferRaw(size_t length, bool doInflate) {
_checkBounds(length);

std::vector<uint8_t> buf(_buf.begin() + _ofs, _buf.begin() + _ofs + length);
_ofs += length;

if (doInflate)
buf = zlib_inflate(buf.data(), buf.size());

return buf;
}

std::string BufferWrapper::readString() {
return readString(remainingBytes());
}

std::string BufferWrapper::readString(size_t length, std::string_view encoding) {
if (length == 0)
return "";

_checkBounds(length);
const uint8_t* ptr = _buf.data() + _ofs;
std::string str;

if (encoding == "ascii") {
	str.resize(length);
	for (size_t i = 0; i < length; ++i)
		str[i] = static_cast<char>(ptr[i] & 0x7F);
} else if (encoding == "utf8" || encoding == "utf-8" || encoding == "latin1" || encoding == "binary") {
	str.assign(reinterpret_cast<const char*>(ptr), length);
} else if (encoding == "base64") {
	str = base64_encode(ptr, length);
} else if (encoding == "hex") {
	str = bytes_to_hex_string(ptr, length);
} else if (encoding == "utf16le" || encoding == "utf-16le" || encoding == "ucs2" || encoding == "ucs-2") {
	str = decode_utf16le_to_utf8(ptr, length);
} else {
	throw std::runtime_error(std::format("readString: unsupported encoding '{}'", encoding));
}

_ofs += length;
return str;
}

std::string BufferWrapper::readNullTerminatedString([[maybe_unused]] std::string_view encoding) {
size_t startPos = _ofs;
size_t length = 0;

while (remainingBytes() > 0) {
if (readUInt8() == 0x0)
break;
length++;
}

seek(static_cast<int64_t>(startPos));

std::string str = readString(length, encoding);
move(1); // Skip the null-terminator.
return str;
}

bool BufferWrapper::startsWith(std::string_view input, [[maybe_unused]] std::string_view encoding) {
seek(0);
return readString(input.size(), encoding) == input;
}

bool BufferWrapper::startsWith(const std::vector<std::string_view>& input, [[maybe_unused]] std::string_view encoding) {
seek(0);
for (const auto& entry : input) {
if (readString(entry.size(), encoding) == entry)
return true;
}
return false;
}

nlohmann::json BufferWrapper::readJSON() {
return readJSON(remainingBytes());
}

nlohmann::json BufferWrapper::readJSON(size_t length, std::string_view encoding) {
return nlohmann::json::parse(readString(length, encoding));
}

std::vector<std::string> BufferWrapper::readLines([[maybe_unused]] std::string_view encoding) {
size_t savedOfs = _ofs;
seek(0);

std::string str = readString(remainingBytes(), encoding);
seek(static_cast<int64_t>(savedOfs));

std::vector<std::string> lines;
size_t start = 0;
for (size_t i = 0; i < str.size(); i++) {
if (str[i] == '\n') {
size_t end = (i > 0 && str[i - 1] == '\r') ? i - 1 : i;
lines.emplace_back(str, start, end - start);
start = i + 1;
}
}
lines.emplace_back(str, start);
return lines;
}

void BufferWrapper::fill(uint8_t value) {
fill(value, remainingBytes());
}

void BufferWrapper::fill(uint8_t value, size_t length) {
_checkBounds(length);
std::memset(_buf.data() + _ofs, value, length);
_ofs += length;
}

// =====================================================================
// Write methods — integers
// =====================================================================

IMPL_WRITE_INT(int8_t,   writeInt8,       1, true)
IMPL_WRITE_INT(uint8_t,  writeUInt8,      1, true)

IMPL_WRITE_INT(int16_t,  writeInt16LE,    2, true)
IMPL_WRITE_INT(uint16_t, writeUInt16LE,   2, true)
IMPL_WRITE_INT(int16_t,  writeInt16BE,    2, false)
IMPL_WRITE_INT(uint16_t, writeUInt16BE,   2, false)

IMPL_WRITE_INT(int32_t,  writeInt24LE,    3, true)
IMPL_WRITE_INT(uint32_t, writeUInt24LE,   3, true)
IMPL_WRITE_INT(int32_t,  writeInt24BE,    3, false)
IMPL_WRITE_INT(uint32_t, writeUInt24BE,   3, false)

IMPL_WRITE_INT(int32_t,  writeInt32LE,    4, true)
IMPL_WRITE_INT(uint32_t, writeUInt32LE,   4, true)
IMPL_WRITE_INT(int32_t,  writeInt32BE,    4, false)
IMPL_WRITE_INT(uint32_t, writeUInt32BE,   4, false)

IMPL_WRITE_INT(int64_t,  writeInt40LE,    5, true)
IMPL_WRITE_INT(uint64_t, writeUInt40LE,   5, true)
IMPL_WRITE_INT(int64_t,  writeInt40BE,    5, false)
IMPL_WRITE_INT(uint64_t, writeUInt40BE,   5, false)

IMPL_WRITE_INT(int64_t,  writeInt48LE,    6, true)
IMPL_WRITE_INT(uint64_t, writeUInt48LE,   6, true)
IMPL_WRITE_INT(int64_t,  writeInt48BE,    6, false)
IMPL_WRITE_INT(uint64_t, writeUInt48BE,   6, false)

IMPL_WRITE_INT(int64_t,  writeBigInt64LE,  8, true)
IMPL_WRITE_INT(uint64_t, writeBigUInt64LE, 8, true)
IMPL_WRITE_INT(int64_t,  writeBigInt64BE,  8, false)
IMPL_WRITE_INT(uint64_t, writeBigUInt64BE, 8, false)

void BufferWrapper::writeFloatLE(float value) {
_checkBounds(4);
uint32_t bits;
std::memcpy(&bits, &value, sizeof(bits));
write_uint_le<4>(_buf.data() + _ofs, bits);
_ofs += 4;
}

void BufferWrapper::writeFloatBE(float value) {
_checkBounds(4);
uint32_t bits;
std::memcpy(&bits, &value, sizeof(bits));
write_uint_be<4>(_buf.data() + _ofs, bits);
_ofs += 4;
}

// =====================================================================
// Write methods — buffer
// =====================================================================

void BufferWrapper::writeBuffer(BufferWrapper& buf, size_t copyLength) {
size_t startIndex = buf.offset();

if (copyLength == 0)
copyLength = buf.remainingBytes();
else
buf._checkBounds(copyLength);

_checkBounds(copyLength);

std::memcpy(_buf.data() + _ofs, buf._buf.data() + startIndex, copyLength);
_ofs += copyLength;
buf._ofs += copyLength;
}

void BufferWrapper::writeBuffer(std::span<const uint8_t> buf, size_t copyLength) {
if (copyLength == 0)
copyLength = buf.size();
else if (buf.size() <= copyLength) {
}

_checkBounds(copyLength);

std::memcpy(_buf.data() + _ofs, buf.data(), copyLength);
_ofs += copyLength;
}

void BufferWrapper::writeToFile(const std::filesystem::path& file) {
std::filesystem::create_directories(file.parent_path());
std::ofstream ofs(file, std::ios::binary);
if (!ofs)
throw std::runtime_error("Failed to open file for writing: " + file.string());

ofs.write(reinterpret_cast<const char*>(_buf.data()), static_cast<std::streamsize>(_buf.size()));
if (!ofs)
throw std::runtime_error("Failed to write file: " + file.string());
}

// =====================================================================
// Search
// =====================================================================

int64_t BufferWrapper::indexOfChar(char ch) {
return indexOf(static_cast<uint8_t>(ch), _ofs);
}

int64_t BufferWrapper::indexOfChar(char ch, size_t start) {
return indexOf(static_cast<uint8_t>(ch), start);
}

int64_t BufferWrapper::indexOf(uint8_t byte) {
return indexOf(byte, _ofs);
}

int64_t BufferWrapper::indexOf(uint8_t byte, size_t start) {
size_t resetPos = _ofs;
seek(static_cast<int64_t>(start));

while (remainingBytes() > 0) {
size_t mark = _ofs;
if (readUInt8() == byte) {
seek(static_cast<int64_t>(resetPos));
return static_cast<int64_t>(mark);
}
}

seek(static_cast<int64_t>(resetPos));
return -1;
}

// =====================================================================
// Utility
// =====================================================================

const std::string& BufferWrapper::getDataURL() {
if (!dataURL)
dataURL = URLPolyfill::createObjectURL(_buf, "");

return *dataURL;
}

bool BufferWrapper::hasDataURL() const {
	return dataURL.has_value();
}

void BufferWrapper::setDataURL(std::string url) {
	dataURL = std::move(url);
}

void BufferWrapper::revokeDataURL() {
	if (dataURL)
		URLPolyfill::revokeObjectURL(*dataURL);
dataURL.reset();
}

std::string BufferWrapper::toBase64() const {
return base64_encode(_buf.data(), _buf.size());
}

BufferWrapper::DecodedAudioData BufferWrapper::decodeAudio() const {
	DecodedAudioData decoded;

	ma_decoder decoder;
	ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 0, 0);
	if (ma_decoder_init_memory(_buf.data(), _buf.size(), &decoderConfig, &decoder) != MA_SUCCESS)
		throw std::runtime_error("decodeAudio: failed to initialize decoder");

	decoded.channels = decoder.outputChannels;
	decoded.sampleRate = decoder.outputSampleRate;

	ma_uint64 frameCount = 0;
	ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
	if (frameCount > 0 && decoded.channels > 0) {
		decoded.samples.resize(static_cast<size_t>(frameCount) * decoded.channels);
		const ma_uint64 framesRead = ma_decoder_read_pcm_frames(&decoder, decoded.samples.data(), frameCount, nullptr);
		decoded.samples.resize(static_cast<size_t>(framesRead) * decoded.channels);
	}

	ma_decoder_uninit(&decoder);
	return decoded;
}

void BufferWrapper::setCapacity(size_t capacity, bool secure) {
if (capacity == byteLength())
return;

std::vector<uint8_t> buf(capacity, 0);
size_t copyLen = std::min(capacity, byteLength());
std::memcpy(buf.data(), _buf.data(), copyLen);
_buf = std::move(buf);
}

std::string BufferWrapper::calculateHash(std::string_view hash, std::string_view encoding) {
const EVP_MD* md = hash_name_to_md(hash);
if (!md)
throw std::runtime_error("calculateHash: unsupported hash algorithm '" + std::string(hash) + "'");

auto digest = evp_hash(_buf.data(), _buf.size(), md);

if (encoding == "hex")
return digest_to_hex(digest);
if (encoding == "base64")
return base64_encode(digest.data(), digest.size());
throw std::runtime_error("calculateHash: unsupported encoding '" + std::string(encoding) + "' (hex or base64)");
}

bool BufferWrapper::isZeroed() const {
for (size_t i = 0, n = byteLength(); i < n; i++) {
if (_buf[i] != 0x0)
return false;
}
return true;
}

uint32_t BufferWrapper::getCRC32() const {
return crc32(std::span<const uint8_t>(_buf));
}

void BufferWrapper::unmapSource() {
if (_mmap) {
#ifdef _WIN32
UnmapViewOfFile(_mmap);
#else
// Use stored mapping size to ensure correct munmap() even after setCapacity().
munmap(_mmap, _mmapSize);
#endif
_mmap = nullptr;
_mmapSize = 0;
}
}

BufferWrapper BufferWrapper::deflate() const {
return BufferWrapper(zlib_deflate(_buf.data(), _buf.size()));
}

// =====================================================================
// Private
// =====================================================================

void BufferWrapper::_checkBounds(size_t length) {
if (remainingBytes() < length)
throw std::runtime_error("Buffer operation out-of-bounds: " +
std::to_string(length) + " > " + std::to_string(remainingBytes()));
}

// Cleanup macros — not needed outside this translation unit.
#undef IMPL_READ_U
#undef IMPL_READ_S
#undef IMPL_WRITE_INT
