#include "screenshotHandler.hpp"

ScreenshotHandler::ScreenshotHandler() {}

void ScreenshotHandler::writeFramebuffer(std::string* hash, std::vector<uint8_t>* jpegBuffer) {
	// Encode the file with libjpeg
	// https://www.ridgesolutions.ie/index.php/2019/12/10/libjpeg-example-encode-jpeg-to-memory-buffer-instead-of-file/
	// https://www.geeksforgeeks.org/hamming-distance-between-two-integers/
	// https://github.com/cascornelissen/dhash-image/blob/master/index.js
	// https://tannerhelland.com/2011/10/01/grayscale-image-algorithm-vb6.html
	// https://raw.githubusercontent.com/libjpeg-turbo/libjpeg-turbo/master/libjpeg.txt
	// https://dev.w3.org/Amaya/libjpeg/example.c
	// https://github.com/LuaDist/libjpeg/blob/master/example.c

	jpeg_compress_struct cinfo;
	jpeg_error_mgr jerr;

	uint8_t row_pointer[heightOfdhashInput][jpegFramebufferScanlineSize];

	cinfo.err       = jpeg_std_error(&jerr);
	jerr.error_exit = [](j_common_ptr cinfo) {
		// Allow it to exit
		(*cinfo->err->output_message)(cinfo);
		jpeg_destroy(cinfo);
		exit(EXIT_FAILURE);
	};
	jerr.output_message = [](j_common_ptr cinfo) {
		char pszErr[JMSG_LENGTH_MAX];
		(*cinfo->err->format_message)(cinfo, pszErr);
		std::string error(pszErr, sizeof(pszErr));
		LOGD << "LIBJPEG WARNING: " << error;
	};

	jpeg_create_compress(&cinfo);

	// Encode to memory
	FILE* outfile = fopen(tempScreenshotName, "wb");
	// unsigned char* jpegBuf;
	// unsigned long jpegSize;
	// jpeg_mem_dest(&cinfo, &jpegBuf, &jpegSize);
	jpeg_stdio_dest(&cinfo, outfile);

	LOGD << "Create mem dest";

	cinfo.image_width      = framebufferWidth;
	cinfo.image_height     = framebufferHeight;
	cinfo.input_components = 3;
	cinfo.in_color_space   = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, jpegQuality, true);

	LOGD << "Set defaults: " << framebufferWidth << " " << framebufferHeight;

	// CRASHES RIGHT HERE
	jpeg_start_compress(&cinfo, true);

	LOGD << "Start compress";

	// Can add a comment section too :)
	const char* comment = "JPEG generated by nx-TAS";
	jpeg_write_marker(&cinfo, JPEG_COM, (const JOCTET*)comment, strlen(comment));

	LOGD << "Write comment marker";

	uint64_t initialPointer = framebufferPointer + FramebufferType::HOME1 * framebufferSize;
	// Encode

	uint8_t dhash[sizeOfDhash];
	int indexByteDhash    = 0;
	uint8_t indexBitDhash = 0;
	// Might need to use this, but would possibly be more inefficent
	// svcMapProcessMemory
	uint64_t unk;
	capsscOpenRawScreenShotReadStream(&unk, &unk, &unk, ViLayerStack_ApplicationForDebug, 1000000);
	while(cinfo.next_scanline < cinfo.image_height) {
		// Obtain data for each row
		uint32_t dataIndex = cinfo.next_scanline * rowSize;
		uint8_t dhashChunk[rowSize * heightOfdhashInput];
		uint64_t bytesRead;
		capsscReadRawScreenShotReadStream(&bytesRead, dhashChunk, sizeof(dhashChunk), dataIndex);
		if(bytesRead != sizeof(dhashChunk)) {
			LOGD << "Dhash chunk read incorrrectly";
		}
		for(int yOffset = 0; yOffset < heightOfdhashInput; yOffset++) {
			for(int x = 0; x < framebufferWidth; x++) {
				uint32_t index = (yOffset * rowSize) + x;

				// Set each value into the color data for this section of the JPEG
				uint16_t startDataIndex = x * jpegBytesPerPixel;

				uint8_t red   = dhashChunk[index];
				uint8_t green = dhashChunk[index + 1];
				uint8_t blue  = dhashChunk[index + 2];

				row_pointer[yOffset][startDataIndex]     = red;
				row_pointer[yOffset][startDataIndex + 1] = green;
				row_pointer[yOffset][startDataIndex + 2] = blue;
			}
		}
		LOGD << "Successfully obtained dHash chunk at y=" << cinfo.next_scanline;
		// Now, calculate the dhash
		int widthOfDhashPixelChunk = framebufferWidth / widthOfdhashInput;
		uint8_t greyscaleRow[widthOfDhashPixelChunk];
		for(int dhashPixelChunk = 0; dhashPixelChunk < widthOfDhashPixelChunk; dhashPixelChunk++) {
			int pixelSumsR = 0;
			int pixelSumsG = 0;
			int pixelSumsB = 0;
			for(int relativeX = 0; relativeX < widthOfdhashInput; relativeX++) {
				for(int relativeY = 0; relativeY < heightOfdhashInput; relativeY++) {
					uint16_t xDataIndex = ((dhashPixelChunk * widthOfdhashInput) + relativeX) * jpegBytesPerPixel;

					// Apparently, it's better to use squaring in this situation, but oh well
					// https://sighack.com/post/averaging-rgb-colors-the-right-way
					pixelSumsR += row_pointer[relativeY][xDataIndex];
					pixelSumsG += row_pointer[relativeY][xDataIndex + 1];
					pixelSumsB += row_pointer[relativeY][xDataIndex + 2];
				}
			}
			int pixelAveragesR = pixelSumsR / (widthOfdhashInput * heightOfdhashInput);
			int pixelAveragesG = pixelSumsG / (widthOfdhashInput * heightOfdhashInput);
			int pixelAveragesB = pixelSumsB / (widthOfdhashInput * heightOfdhashInput);
			// Get greyscale value with a simple formula
			greyscaleRow[dhashPixelChunk] = (pixelAveragesR + pixelAveragesG + pixelAveragesB) / 3;
			// Don't use the first row yet
			if(dhashPixelChunk != 0) {
				// Use the comparison to set the bit
				bool isLeftBrighter = greyscaleRow[dhashPixelChunk - 1] < greyscaleRow[dhashPixelChunk];
				SET_BIT(dhash[indexByteDhash], isLeftBrighter, indexBitDhash);
				if(indexBitDhash == 7) {
					// Loop around
					indexBitDhash = 0;
					indexByteDhash++;
				} else {
					indexBitDhash++;
				}
			}
		}

		// Now, write the JPEG
		int scanlinesActuallyWritten = jpeg_write_scanlines(&cinfo, row_pointer, heightOfdhashInput);
		if(scanlinesActuallyWritten != heightOfdhashInput) {
			// Un oh
			LOGD << "Scanlines wrong in JPEG";
		}
	}
	capsscCloseRawScreenShotReadStream();

	LOGD << "Successfully wrote JPEG";

	jpeg_finish_compress(&cinfo);

	// Now that JPEG and dhash are done, send them both back
	hash->assign(convertToHexString(dhash, sizeOfDhash));
	// jpegBuffer->resize(jpegSize);
	// memcpy(jpegBuffer->data(), jpegBuf, jpegSize);

	jpeg_destroy_compress(&cinfo);
	fclose(outfile);

	// Read all data into memory
	hash->assign(convertToHexString(dhash, sizeOfDhash));

	jpegBuffer->clear();
	std::ifstream file(tempScreenshotName, std::ios::binary);
	std::copy(std::istream_iterator<uint8_t>(file), std::istream_iterator<uint8_t>(), std::back_inserter(*jpegBuffer));
	file.close();
	// remove(tempScreenshotName);
}

std::string ScreenshotHandler::convertToHexString(uint8_t* data, uint16_t size) {
	// https://codereview.stackexchange.com/a/78539
	constexpr char hexmap[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	std::string s(size * 2, ' ');
	for(int i = 0; i < size; ++i) {
		s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
		s[2 * i + 1] = hexmap[data[i] & 0x0F];
	}
	return s;
}