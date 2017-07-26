#include "IoContext.h"

namespace core
{
IoContext::IoContext(Client * client, IoType io_type) : client_(client), io_type_(io_type)
{
	SecureZeroMemory(&overlapped_, sizeof(overlapped_));
	SecureZeroMemory(&buffer_, sizeof(buffer_));
	buffer_.len = 0;
	buffer_.buf = nullptr;
}
IoContext::~IoContext()
{
}
}