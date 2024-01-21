#ifndef SPOT_BASE_NONCOPYABLE_H
#define SPOT_BASE_NONCOPYABLE_H
namespace spot 
{
namespace utility
{
	class NonCopyable
	{
	protected:
		NonCopyable() = default;
		~NonCopyable() = default;
		
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;
		
	};
}
}

#endif