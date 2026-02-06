
#include <string>
#include <vector>

#include <inttypes.h> 


inline void insertString(std::vector<unsigned char>& v, char* b)
{
	v.push_back(strlen(b));
	for (int x = 0; x < (strlen(b)); x++) v.push_back(b[x]);
}

inline void insertDouble(std::vector<unsigned char>& v, double d)
{
	uint64_t i64rep;
	memcpy(&i64rep, &d, sizeof(double));
	unsigned char* p = (unsigned char*)&i64rep;
	for (int i = 0; i < 8; i++) v.push_back(p[i]);
}

inline std::vector<uint8_t> encodeVarint(uint64_t value) {
	std::vector<uint8_t> result;
	unsigned int uvalue = static_cast<uint64_t>(value); // Use unsigned for bitwise operations

	while (uvalue >= 0x80) { // While value is 7 bits or more
		result.push_back(static_cast<uint8_t>((uvalue & 0x7F) | 0x80)); // Take 7 bits, set MSB to 1
		uvalue >>= 7; // Shift right by 7 bits
	}
	result.push_back(static_cast<uint8_t>(uvalue & 0x7F)); // Add the last 7 bits (MSB 0)

	printf("Encoding %" PRIu64 "\r\n", value);

	for (int i = 0; i < result.size(); i++)
	{
		printf("%02X ", result[i]);
	}
	puts("");

	return result;
}

// Encodes a uint64_t into a varint byte sequence and returns it as a vector of uint8_t.
inline std::vector<uint8_t> encodeVarint64(uint64_t value) {
	std::vector<uint8_t> encoded_bytes;
	do {
		uint8_t byte = value & 0x7F; // Get the lower 7 bits
		value >>= 7;                // Shift the value to the right by 7 bits
		if (value > 0) {
			byte |= 0x80; // Set the MSB if more bytes are to follow
		}
		encoded_bytes.push_back(byte);
	} while (value > 0);
	return encoded_bytes;
}




