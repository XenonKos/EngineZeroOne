#pragma once
#include "UploadBuffer.h"

template <UINT Size>
class ShaderRecord {
public:
	void SetIdentifier(void* ptr);
private:
	uint8_t* ptr;
};

template <typename RecordType>
class ShaderTable : public UploadBuffer<RecordType> {
public:

private:

};

template<UINT Size>
inline void ShaderRecord<Size>::SetIdentifier(void* ptr)
{
}
