#include "MPOParser.h"

#define safe_delete(x) {if(x!=NULL)delete[]x;x=NULL;}

MpoParser::MpoParser()
{
	m_NumberOfImages = 0;
	memset(m_datas, 0, sizeof(m_datas));
}

MpoParser::~MpoParser()
{
	for(int i=0; i<m_NumberOfImages; i++)
		safe_delete(m_datas[i]);
}

int MpoParser::parseFile(const wchar_t *path)
{
	static const unsigned short MARKER_SOI = 0xFFD8;
	static const unsigned short MARKER_APP1 = 0xFFE1;
	static const unsigned short MARKER_APP2 = 0xFFE2;
	static const unsigned short MARKER_DQT = 0xFFDB;
	static const unsigned short MARKER_DHT = 0xFFC4;
	static const unsigned short MARKER_DRI = 0xFFDD;
	static const unsigned short MARKER_SOF = 0xFFC0;
	static const unsigned short MARKER_SOS = 0xFFDA;
	static const unsigned short MARKER_EOI = 0xFFD9;
	static const unsigned int MP_LITTLE_ENDIAN = 0x49492A00;

	m_NumberOfImages = 0;
	for(int i=0; i<m_NumberOfImages; i++)
		safe_delete(m_datas[i]);

	FILE *r = _wfopen(path, L"rb");
	if (!r)
		return 0;

	unsigned short soi = read2(r, false);
	if (soi != MARKER_SOI)
	{
		fclose(r);
		return 0;
	}

	while (ftell(r) < fsize(r))
	{
		unsigned short marker = read2(r, false);
		unsigned short length = read2(r, false);

		if (marker == MARKER_APP2)
		{
			// Read APP2 block
			char identifier[5] = {0};
			fread(identifier, 1, 4, r);
			length -= 4;

			if (strcmp(identifier, "MPF\0") == 0)
			{
				// Read MP Extensions
				unsigned int startOfOffset = ftell(r);
				unsigned int mpEndian = read4(r, false);
				bool isLittleEndian = mpEndian == MP_LITTLE_ENDIAN;
				unsigned int offsetToFirstIFD = read4(r, isLittleEndian);
				fseek(r, startOfOffset + offsetToFirstIFD, SEEK_SET);

				unsigned short count = read2(r, isLittleEndian);

				// Read the MP Index IFD
				char version[5] = {0};
				m_NumberOfImages = 0;
				unsigned int mpEntry = 0;
				unsigned int imageUIDList = 0;
				unsigned int totalFrames = 0;
				for (int i = 0; i < count; i++)
				{
					unsigned short tag = read2(r, isLittleEndian);
					unsigned short type = read2(r, isLittleEndian);
					unsigned int count2 = read4(r, isLittleEndian);
					switch (tag)
					{
					case 0xB000:
						fread(version, 1, 4, r);
						break;
					case 0xB001:
						m_NumberOfImages = read4(r, isLittleEndian);
						break;
					case 0xB002:
						mpEntry = read4(r, isLittleEndian);
						break;
					case 0xB003:
						imageUIDList = read4(r, isLittleEndian);
						break;
					case 0xB004:
						totalFrames = read4(r, isLittleEndian);
						break;
					}
				}

				unsigned int offsetNext = read4(r, isLittleEndian);

				// Read the values of the MP Index IFD
				for (int i = 0; i < m_NumberOfImages; i++)
				{
					// var bytes = r.ReadBytes(16);
					unsigned int iattr = read4(r, isLittleEndian);
					unsigned int imageSize = read4(r, isLittleEndian);
					unsigned int dataOffset = read4(r, isLittleEndian);
					unsigned short d1EntryNo = read2(r, isLittleEndian);
					unsigned short d2EntryNo = read2(r, isLittleEndian);

					// Calculate offset from beginning of file
					long offset = i == 0 ? 0 : dataOffset + startOfOffset;

					// store the current position
					long o = ftell(r);

					// read the image
					fseek(r, offset, SEEK_SET);
					m_datas[i] = new char[imageSize];
					m_sizes[i] = imageSize;
					if (fread(m_datas[i], 1, imageSize, r) != imageSize)
						return (m_NumberOfImages=0);

					// restore the current position                                    
					fseek(r, o, SEEK_SET);
				}

				fclose(r);
				return m_NumberOfImages;
			}
		}
		fseek(r, length-2, SEEK_CUR);
	}

	fclose(r);
	return m_NumberOfImages;
}

unsigned short MpoParser::read2(FILE *f, bool isLittleEndian)
{
	unsigned short o = 0;

	fread(&o, 1, 2, f);

	if (!isLittleEndian)
		swapendian(&o, 2);

	return o;
}

unsigned int MpoParser::read4(FILE *f, bool isLittleEndian)
{
	unsigned int o = 0;

	fread(&o, 1, 4, f);

	if (!isLittleEndian)
		swapendian(&o, 4);

	return o;
}

void MpoParser::swapendian(void *p, int size)
{
	char *t = (char*)p;
	for(int i=0; i<size/2; i++)
	{
		t[i] = t[size-i-1] ^ t[i];
		t[size-i-1] = t[i] ^ t[size-i-1];
		t[i] = t[size-i-1] ^ t[i];
	}
}	
int MpoParser::fsize(FILE *f)
{
	unsigned int pos = ftell(f);
	fseek(f, 0, SEEK_END);
	unsigned int o = ftell(f);
	fseek(f, pos, SEEK_SET);
	return o;
}