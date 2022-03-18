#include "Buffer.h"
#include"BufferView.h"

BufferView::BufferView(Buffer const* buffer)
	: buffer(buffer),
	offset(0) {}

BufferView::BufferView(Buffer const* buffer,uint64 offset,uint64 byteSize)
	: buffer(buffer),
	offset(offset),
	byteSize(byteSize) {}

Buffer::Buffer(Device* device)
	: Resource(device) {}

Buffer::~Buffer() {}
