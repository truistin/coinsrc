#ifndef SPOT_BASE_BOOSTGZIP_H
#define SPOT_BASE_BOOSTGZIP_H

#include <sstream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
namespace spot
{
	namespace base
	{
		class BoostGZip {
		public:
			static std::string compress(const std::string& data)
			{
				namespace bio = boost::iostreams;
				std::stringstream compressed;
				std::stringstream origin(data);

				bio::filtering_streambuf<bio::input> out;
				out.push(bio::gzip_compressor{});
				out.push(origin);
				bio::copy(out, compressed);
				return compressed.str();
			}

			static std::string decompress(const std::string& data)
			{
				namespace bio = boost::iostreams;

				std::stringstream compressed(data);
				std::stringstream decompressed;

				bio::filtering_streambuf<bio::input> out;
				out.push(bio::gzip_decompressor{});
				out.push(compressed);
				bio::copy(out, decompressed);
				return decompressed.str();
			}
		};
	}
}

#endif // __GZIP_H__