/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
License: MIT
 */

#include "buffer.h"
#include "crc32.h"

#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <array>

#include <zlib.h>

#include <nlohmann/json.hpp>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace {

// ─── Endian-aware read/write helpers ────────────────────────────────

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
constexpr uint64_t mask = (N < 8) ? ((1ULL << (N * 8)) - 1) : ~0ULL;
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

// ─── Base64 helpers ─────────────────────────────────────────────────

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

// ─── MD5 implementation (RFC 1321) ──────────────────────────────────

constexpr uint32_t md5_s[64] = {
7,12,17,22, 7,12,17,22, 7,12,17,22, 7,12,17,22,
5, 9,14,20, 5, 9,14,20, 5, 9,14,20, 5, 9,14,20,
4,11,16,23, 4,11,16,23, 4,11,16,23, 4,11,16,23,
6,10,15,21, 6,10,15,21, 6,10,15,21, 6,10,15,21
};

constexpr uint32_t md5_k[64] = {
0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

inline uint32_t md5_left_rotate(uint32_t x, uint32_t c) {
return (x << c) | (x >> (32 - c));
}

void md5_transform(uint32_t state[4], const uint8_t block[64]) {
uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
uint32_t M[16];

for (int i = 0; i < 16; i++) {
M[i] = static_cast<uint32_t>(block[i * 4]) |
(static_cast<uint32_t>(block[i * 4 + 1]) << 8) |
(static_cast<uint32_t>(block[i * 4 + 2]) << 16) |
(static_cast<uint32_t>(block[i * 4 + 3]) << 24);
}

for (int i = 0; i < 64; i++) {
uint32_t f, g;
if (i < 16) {
f = (b & c) | (~b & d);
g = static_cast<uint32_t>(i);
} else if (i < 32) {
f = (d & b) | (~d & c);
g = static_cast<uint32_t>((5 * i + 1) % 16);
} else if (i < 48) {
f = b ^ c ^ d;
g = static_cast<uint32_t>((3 * i + 5) % 16);
} else {
f = c ^ (b | ~d);
g = static_cast<uint32_t>((7 * i) % 16);
}

f = f + a + md5_k[i] + M[g];
a = d;
d = c;
c = b;
b = b + md5_left_rotate(f, md5_s[i]);
}

state[0] += a;
state[1] += b;
state[2] += c;
state[3] += d;
}

std::string md5_hex(const uint8_t* data, size_t len) {
uint32_t state[4] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 };

size_t i = 0;
for (; i + 64 <= len; i += 64)
md5_transform(state, data + i);

uint8_t block[64];
size_t remaining = len - i;
std::memcpy(block, data + i, remaining);
block[remaining] = 0x80;

if (remaining >= 56) {
std::memset(block + remaining + 1, 0, 63 - remaining);
md5_transform(state, block);
std::memset(block, 0, 56);
} else {
std::memset(block + remaining + 1, 0, 55 - remaining);
}

uint64_t bits = static_cast<uint64_t>(len) * 8;
for (int j = 0; j < 8; j++)
block[56 + j] = static_cast<uint8_t>(bits >> (j * 8));

md5_transform(state, block);

constexpr char hex_chars[] = "0123456789abcdef";
std::string result;
result.reserve(32);
for (int j = 0; j < 4; j++) {
for (int k = 0; k < 4; k++) {
uint8_t byte = static_cast<uint8_t>(state[j] >> (k * 8));
result += hex_chars[byte >> 4];
result += hex_chars[byte & 0xf];
}
}
return result;
}

std::string md5_base64(const uint8_t* data, size_t len) {
uint32_t state[4] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 };

size_t i = 0;
for (; i + 64 <= len; i += 64)
md5_transform(state, data + i);

uint8_t block[64];
size_t remaining = len - i;
std::memcpy(block, data + i, remaining);
block[remaining] = 0x80;

if (remaining >= 56) {
std::memset(block + remaining + 1, 0, 63 - remaining);
md5_transform(state, block);
std::memset(block, 0, 56);
} else {
std::memset(block + remaining + 1, 0, 55 - remaining);
}

uint64_t bits = static_cast<uint64_t>(len) * 8;
for (int j = 0; j < 8; j++)
block[56 + j] = static_cast<uint8_t>(bits >> (j * 8));

md5_transform(state, block);

uint8_t digest[16];
for (int j = 0; j < 4; j++) {
for (int k = 0; k < 4; k++)
digest[j * 4 + k] = static_cast<uint8_t>(state[j] >> (k * 8));
}
return base64_encode(digest, 16);
}

// ─── SHA1 implementation (RFC 3174) ─────────────────────────────────

inline uint32_t sha1_left_rotate(uint32_t x, uint32_t n) {
	return (x << n) | (x >> (32 - n));
}

void sha1_transform(uint32_t state[5], const uint8_t block[64]) {
	uint32_t w[80];

	for (int i = 0; i < 16; i++) {
		w[i] = (static_cast<uint32_t>(block[i * 4]) << 24) |
			   (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
			   (static_cast<uint32_t>(block[i * 4 + 2]) << 8) |
			    static_cast<uint32_t>(block[i * 4 + 3]);
	}

	for (int i = 16; i < 80; i++)
		w[i] = sha1_left_rotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

	uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];

	for (int i = 0; i < 80; i++) {
		uint32_t f, k;
		if (i < 20) {
			f = (b & c) | (~b & d);
			k = 0x5A827999;
		} else if (i < 40) {
			f = b ^ c ^ d;
			k = 0x6ED9EBA1;
		} else if (i < 60) {
			f = (b & c) | (b & d) | (c & d);
			k = 0x8F1BBCDC;
		} else {
			f = b ^ c ^ d;
			k = 0xCA62C1D6;
		}

		uint32_t temp = sha1_left_rotate(a, 5) + f + e + k + w[i];
		e = d;
		d = c;
		c = sha1_left_rotate(b, 30);
		b = a;
		a = temp;
	}

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
}

void sha1_compute(const uint8_t* data, size_t len, uint32_t state[5]) {
	state[0] = 0x67452301;
	state[1] = 0xEFCDAB89;
	state[2] = 0x98BADCFE;
	state[3] = 0x10325476;
	state[4] = 0xC3D2E1F0;

	size_t i = 0;
	for (; i + 64 <= len; i += 64)
		sha1_transform(state, data + i);

	uint8_t block[64];
	size_t remaining = len - i;
	std::memcpy(block, data + i, remaining);
	block[remaining] = 0x80;

	if (remaining >= 56) {
		std::memset(block + remaining + 1, 0, 63 - remaining);
		sha1_transform(state, block);
		std::memset(block, 0, 56);
	} else {
		std::memset(block + remaining + 1, 0, 55 - remaining);
	}

	// SHA1 uses big-endian bit length
	uint64_t bits = static_cast<uint64_t>(len) * 8;
	block[56] = static_cast<uint8_t>(bits >> 56);
	block[57] = static_cast<uint8_t>(bits >> 48);
	block[58] = static_cast<uint8_t>(bits >> 40);
	block[59] = static_cast<uint8_t>(bits >> 32);
	block[60] = static_cast<uint8_t>(bits >> 24);
	block[61] = static_cast<uint8_t>(bits >> 16);
	block[62] = static_cast<uint8_t>(bits >> 8);
	block[63] = static_cast<uint8_t>(bits);

	sha1_transform(state, block);
}

std::string sha1_hex(const uint8_t* data, size_t len) {
	uint32_t state[5];
	sha1_compute(data, len, state);

	constexpr char hex_chars[] = "0123456789abcdef";
	std::string result;
	result.reserve(40);
	for (int j = 0; j < 5; j++) {
		for (int k = 3; k >= 0; k--) {
			uint8_t byte = static_cast<uint8_t>(state[j] >> (k * 8));
			result += hex_chars[byte >> 4];
			result += hex_chars[byte & 0xf];
		}
	}
	return result;
}

std::string sha1_base64(const uint8_t* data, size_t len) {
	uint32_t state[5];
	sha1_compute(data, len, state);

	uint8_t digest[20];
	for (int j = 0; j < 5; j++) {
		digest[j * 4]     = static_cast<uint8_t>(state[j] >> 24);
		digest[j * 4 + 1] = static_cast<uint8_t>(state[j] >> 16);
		digest[j * 4 + 2] = static_cast<uint8_t>(state[j] >> 8);
		digest[j * 4 + 3] = static_cast<uint8_t>(state[j]);
	}
	return base64_encode(digest, 20);
}

// ─── zlib helpers ───────────────────────────────────────────────────

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
std::vector<uint8_t> buf(length);
if (!secure) {
// Insecure alloc — contents uninitialized in the JS sense.
// std::vector value-initializes to zero anyway, which is fine.
}
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
: _ofs(other._ofs), _buf(std::move(other._buf)), _mmap(other._mmap), dataURL(std::move(other.dataURL)) {
other._ofs = 0;
other._mmap = nullptr;
}

BufferWrapper& BufferWrapper::operator=(BufferWrapper&& other) noexcept {
if (this != &other) {
_ofs = other._ofs;
_buf = std::move(other._buf);
_mmap = other._mmap;
dataURL = std::move(other.dataURL);
other._ofs = 0;
other._mmap = nullptr;
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

std::string BufferWrapper::readString(size_t length) {
if (length == 0)
return "";

_checkBounds(length);
std::string str(reinterpret_cast<const char*>(_buf.data() + _ofs), length);
_ofs += length;
return str;
}

std::string BufferWrapper::readNullTerminatedString() {
size_t startPos = _ofs;
size_t length = 0;

while (remainingBytes() > 0) {
if (readUInt8() == 0x0)
break;
length++;
}

seek(static_cast<int64_t>(startPos));

std::string str = readString(length);
move(1); // Skip the null-terminator.
return str;
}

bool BufferWrapper::startsWith(std::string_view input) {
seek(0);
return readString(input.size()) == input;
}

bool BufferWrapper::startsWith(const std::vector<std::string_view>& input) {
seek(0);
for (const auto& entry : input) {
if (readString(entry.size()) == entry)
return true;
}
return false;
}

nlohmann::json BufferWrapper::readJSON() {
return readJSON(remainingBytes());
}

nlohmann::json BufferWrapper::readJSON(size_t length) {
return nlohmann::json::parse(readString(length));
}

std::vector<std::string> BufferWrapper::readLines() {
size_t savedOfs = _ofs;
seek(0);

std::string str = readString(remainingBytes());
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
else if (buf.size() < copyLength)
throw std::runtime_error("Buffer operation out-of-bounds: " +
std::to_string(copyLength) + " > " + std::to_string(buf.size()));

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
dataURL = "data:application/octet-stream;base64," + toBase64();

return *dataURL;
}

void BufferWrapper::revokeDataURL() {
dataURL.reset();
}

std::string BufferWrapper::toBase64() const {
return base64_encode(_buf.data(), _buf.size());
}

void BufferWrapper::setCapacity(size_t capacity, bool secure) {
if (capacity == byteLength())
return;

std::vector<uint8_t> buf(capacity, 0);
if (!secure) {
// In JS, Buffer.allocUnsafe doesn't zero. std::vector always zeroes
// with value initialization. This is a harmless difference.
}

size_t copyLen = std::min(capacity, byteLength());
std::memcpy(buf.data(), _buf.data(), copyLen);
_buf = std::move(buf);
}

std::string BufferWrapper::calculateHash(std::string_view hash, std::string_view encoding) {
if (hash != "md5" && hash != "sha1")
throw std::runtime_error("calculateHash: unsupported hash algorithm '" + std::string(hash) + "' (only md5, sha1 supported)");

if (encoding == "hex") {
if (hash == "sha1")
return sha1_hex(_buf.data(), _buf.size());
return md5_hex(_buf.data(), _buf.size());
} else if (encoding == "base64") {
if (hash == "sha1")
return sha1_base64(_buf.data(), _buf.size());
return md5_base64(_buf.data(), _buf.size());
} else {
throw std::runtime_error("calculateHash: unsupported encoding '" + std::string(encoding) + "' (only hex, base64 supported)");
}
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
// Size must match the original mapping; _buf.size() equals the
// size passed to fromMmap() as long as setCapacity() was not called.
munmap(_mmap, _buf.size());
#endif
_mmap = nullptr;
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
