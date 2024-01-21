#ifndef __OK_ZLIB_WRAPPER_H__
#define __OK_ZLIB_WRAPPER_H__

#include <sstream>
#include <string.h>
//#include <zlib.h>
#include <zlib/include/zlib.h>

class OkZlibWrapper {
public:
	static std::string decompress(const std::string& data)
	{
		static char pBuf[1024 * 1024];
		memset(pBuf, 0, sizeof(pBuf));
		unsigned long nSize = 1024 * 1024;

		if (gzdecompress((unsigned char *)data.c_str(), data.size(), (unsigned char*)pBuf, &nSize)) {
			return "";
		}
		if (nSize >= 2) { //okex上的json串有时候会丢失最后一个']'字符
			if ('[' == pBuf[0] && ']' != pBuf[nSize - 1]) {
				pBuf[nSize] = ']';
				++nSize;
			}
			if ('{' == pBuf[0] && '}' != pBuf[nSize - 1]) {
				pBuf[nSize] = '}';
				++nSize;
			}
		}
		return std::string(pBuf, nSize);
	}

	/*!
	* \brief gzip数据解压。
	* \param zdata 要解压的数据
	* \param nzdata 原数据长度
	* \param data 解压后数据
	* \param ndata 解压后长度（注意：输入值为用于接收结果的buffer长度，即参数data的大小。）
	* \return 返回0表示成功，否则失败。
	*/
	static int gzdecompress(unsigned char *zdata, unsigned long nzdata, unsigned char *data, unsigned long *ndata)
	{
		int err = 0;
		z_stream d_stream = { 0 }; /* decompression stream */

		static char dummy_head[2] = {
			0x8 + 0x7 * 0x10,
			(((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
		};

		d_stream.zalloc = NULL;
		d_stream.zfree = NULL;
		d_stream.opaque = NULL;
		d_stream.next_in = zdata;
		d_stream.avail_in = 0;
		d_stream.next_out = data;

		// 只有设置为MAX_WBITS + 16才能在解压带header和trailer的文本
		if (inflateInit2(&d_stream, -MAX_WBITS) != Z_OK) {
			return -1;
		}

		// if(inflateInit2(&d_stream, 47) != Z_OK) return -1;

		while (d_stream.total_out < *ndata && d_stream.total_in < nzdata) {
			d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
			if ((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END)
				break;

			if (err != Z_OK) {
				if (err == Z_DATA_ERROR) {
					d_stream.next_in = (Bytef*)dummy_head;
					d_stream.avail_in = sizeof(dummy_head);
					if ((err = inflate(&d_stream, Z_NO_FLUSH)) != Z_OK) {
						return -1;
					}
				}
				else {
					return -1;
				}
			}
		}

		if (inflateEnd(&d_stream) != Z_OK) {
			return -1;
		}
		*ndata = d_stream.total_out;
		return 0;
	}
};

#endif // __OK_ZLIB_WRAPPER_H__
