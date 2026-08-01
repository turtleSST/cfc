/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

// The v2 header preserves the original first six bytes for small files while
// adding seven high size bits and a format marker.  This keeps the color
// calibration layout stable and expands the length from 25 to 32 bits.
class FountainMetadata
{
protected:
	static constexpr uint8_t format_marker = 0xC2;

	static void update_block_id_internal(uint16_t block_id, uint8_t* arr)
	{
		arr[0] = (block_id >> 8) & 0xFF;
		arr[1] = block_id & 0xFF;
	}

public:
	static constexpr unsigned md_size = 8;
	static constexpr unsigned legacy_md_size = 6;
	static constexpr uint32_t max_file_size = UINT32_MAX;

	static void to_uint8_arr(uint8_t encode_id, uint64_t size, uint16_t block_id, uint8_t* arr)
	{
		// The stream classes reject larger inputs before they reach this method.
		uint32_t file_size = static_cast<uint32_t>(size);
		arr[0] = (encode_id & 0x7F) | ((file_size >> 17) & 0x80);
		arr[1] = (file_size >> 16) & 0xFF;
		arr[2] = (file_size >> 8) & 0xFF;
		arr[3] = file_size & 0xFF;
		update_block_id_internal(block_id, arr + 4);
		arr[6] = (file_size >> 25) & 0x7F;
		arr[7] = format_marker;
	}

public:
	// The public id is deliberately independent of the byte layout.  It is
	// passed through the JS API, so it must retain all bits of a 32-bit size.
	FountainMetadata(uint64_t id)
	{
		uint8_t* d = _data.data();
		to_uint8_arr(static_cast<uint8_t>((id >> 32) & 0x7F), static_cast<uint32_t>(id), 0, d);
	}

	explicit FountainMetadata(const char* buff, unsigned len)
	{
		_data.fill(0);
		if (len > md_size)
			len = md_size;
		std::copy(buff, buff + len, _data.data());
	}

	static FountainMetadata from_legacy(const char* buff, unsigned len)
	{
		FountainMetadata md(buff, len);
		md._legacy = true;
		return md;
	}

	FountainMetadata(uint8_t encode_id, uint32_t size, uint16_t block_id)
	{
		uint8_t* d = _data.data();
		to_uint8_arr(encode_id, size, block_id, d);
	}

	uint64_t id() const
	{
		return (static_cast<uint64_t>(encode_id()) << 32) | file_size();
	}

	bool valid() const
	{
		return _legacy ? file_size() != 0 : data()[7] == format_marker;
	}

	bool is_legacy() const
	{
		return _legacy;
	}

	static bool looks_like_v2(const char* buff, unsigned len)
	{
		return len >= md_size && static_cast<uint8_t>(buff[7]) == format_marker;
	}

	uint8_t encode_id() const
	{
		return data()[0] & 0x7F;
	}

	uint16_t block_id() const
	{
		const uint8_t* d = data() + 4;
		return (static_cast<uint16_t>(d[0]) << 8) | d[1];
	}

	void increment_block_id(unsigned radioactive_block_id)
	{
		unsigned next = block_id() + 1;
		if (next == radioactive_block_id)
			next += 1;
		update_block_id_internal(static_cast<uint16_t>(next), _data.data() + 4);
	}

	uint32_t file_size() const
	{
		const uint8_t* d = data();
		uint32_t res = d[3];
		res |= static_cast<uint32_t>(d[2]) << 8;
		res |= static_cast<uint32_t>(d[1]) << 16;
		res |= static_cast<uint32_t>(d[0] & 0x80) << 17;
		if (_legacy)
			return res;
		res |= static_cast<uint32_t>(d[6] & 0x7F) << 25;
		return res;
	}

	const uint8_t* data() const
	{
		return _data.data();
	}

protected:
	std::array<uint8_t, md_size> _data{};
	bool _legacy = false;
};
