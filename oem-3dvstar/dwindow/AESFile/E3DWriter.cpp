#include <stdio.h>
#include <windows.h>
#include "E3DReader.h"
#include "E3DWriter.h"

const int E3D_BLOCK_SIZE = 16;

class my12doom_leaf
{
public:
	__int64 m_leaf8cc;
	__int64 m_child_size;

	enum
	{
		leaf_is_not_set,
		leaf_is_memory,
		leaf_is_file_pos,
		leaf_is_childs,
	} m_leaf_type;

	struct  
	{
		void * data;
		__int64 size;
	} m_memory_data;

	struct
	{
		HANDLE file;
		LONGLONG pos;
		LONGLONG size;
	} m_file_data;

	int m_leaves_count;
	my12doom_leaf **m_leaves;
	my12doom_leaf();
	~my12doom_leaf();
	void AddLeaf(my12doom_leaf* leaf);
	void SetData(HANDLE file, LONGLONG pos, LONGLONG size);
	void SetData(void *data, LONGLONG size);
	void WriteTo(HANDLE file);
	void WriteTo(FILE *file);
};

my12doom_leaf::my12doom_leaf()
{
	m_child_size = 0;
	m_leaf8cc = 0;
	m_leaves_count = 0;
	m_leaf_type = leaf_is_not_set;
	m_leaves = NULL;
}

my12doom_leaf::~my12doom_leaf()
{
	if (m_leaves)
		free(m_leaves);
}

void my12doom_leaf::AddLeaf(my12doom_leaf* leaf)
{
	if (m_leaf_type != leaf_is_not_set && m_leaf_type != leaf_is_childs)
		return;

	m_leaf_type = leaf_is_childs;
	m_child_size += 16 + leaf->m_child_size;

	m_leaves_count++;
	m_leaves = (my12doom_leaf**)realloc(m_leaves, m_leaves_count*sizeof(my12doom_leaf**));
	m_leaves[m_leaves_count-1] = leaf;
}

void my12doom_leaf::SetData(void *data, LONGLONG size)
{
	m_leaf_type = leaf_is_memory;
	m_child_size = size;
	m_memory_data.data = data;
	m_memory_data.size = size;
}

void my12doom_leaf::SetData(HANDLE file, LONGLONG pos, LONGLONG size)
{
	m_leaf_type = leaf_is_file_pos;
	m_child_size = size;
	m_file_data.file = file;
	m_file_data.pos = pos;
	m_file_data.size = size;
}

void my12doom_leaf::WriteTo(HANDLE file)
{
	DWORD byte_written = 0;
	WriteFile(file, &m_leaf8cc, 8, &byte_written, NULL);
	WriteFile(file, &m_child_size, 8, &byte_written, NULL);

	if (m_leaf_type == leaf_is_childs)
	{
		for(int i=0; i<m_leaves_count; i++)
			m_leaves[i]->WriteTo(file);
	}

	else if (m_leaf_type == leaf_is_memory)
	{
		WriteFile(file, m_memory_data.data, (DWORD)m_memory_data.size, &byte_written, NULL);
	}

	else if (m_leaf_type == leaf_is_file_pos)
	{
		char tmp[E3D_BLOCK_SIZE];
		LARGE_INTEGER newpos;
		newpos.QuadPart = m_file_data.pos;
		SetFilePointerEx(m_file_data.file, newpos, NULL, SEEK_SET);
		for(__int64 byte_left= m_file_data.size; byte_left>0; byte_left-= E3D_BLOCK_SIZE)
		{
			DWORD byte_read = 0;
			ReadFile(m_file_data.file, tmp, (DWORD)byte_left, &byte_read, NULL);
			WriteFile(file, tmp, byte_read, &byte_written, NULL);

			if (byte_read < byte_left)
				break;
		}
	}
}

void my12doom_leaf::WriteTo(FILE *file)
{
	fwrite(&m_leaf8cc, 1, 8, file);
	fwrite(&m_child_size, 1, 8, file);

	if (m_leaf_type == leaf_is_childs)
	{
		for(int i=0; i<m_leaves_count; i++)
			m_leaves[i]->WriteTo(file);
	}

	else if (m_leaf_type == leaf_is_memory)
	{
		fwrite(m_memory_data.data, 1, (size_t)m_memory_data.size, file);
	}

	else if (m_leaf_type == leaf_is_file_pos)
	{
		char tmp[E3D_BLOCK_SIZE];
		LARGE_INTEGER newpos;
		newpos.QuadPart = m_file_data.pos;
		SetFilePointerEx(m_file_data.file, newpos, NULL, SEEK_SET);
		for(__int64 byte_left= m_file_data.size; byte_left>0; byte_left-= E3D_BLOCK_SIZE)
		{
			DWORD byte_read = 0;
			ReadFile(m_file_data.file, tmp, (DWORD)byte_left, &byte_read, NULL);
			fwrite(tmp, 1, byte_read, file);

			if (byte_read < byte_left)
				break;
		}
	}

}


int encode_file(wchar_t *in, wchar_t *out, unsigned char *key, unsigned char *source_hash, E3D_Writer_Progress *cb)
{
	HANDLE hin = CreateFileW (in, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	HANDLE hout = CreateFileW (out, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);

	if (hin == INVALID_HANDLE_VALUE || hout == INVALID_HANDLE_VALUE)
		return -1;


	// root
	my12doom_leaf root;
	root.m_leaf8cc = str2int64("my12doom");

	// file size
	LARGE_INTEGER lint;
	GetFileSizeEx(hin, &lint);
	__int64 file_size = lint.QuadPart;
	my12doom_leaf leaf_file_size;
	leaf_file_size.m_leaf8cc = str2int64("filesize");
	leaf_file_size.SetData(&file_size, 8);
	root.AddLeaf(&leaf_file_size);

	// block size
	__int64 block_size = E3D_BLOCK_SIZE;
	my12doom_leaf leaf_block_size;
	leaf_block_size.m_leaf8cc = str2int64("blocksize");
	leaf_block_size.SetData(&block_size, 8);
	root.AddLeaf(&leaf_block_size);

	// layout
	__int64 layout = 0;
	my12doom_leaf leaf_layout;
	leaf_layout.m_leaf8cc = str2int64("layout");
	leaf_layout.SetData(&layout, 8);
	root.AddLeaf(&leaf_layout);

	// source hash
	my12doom_leaf leaf_hash;
	leaf_hash.m_leaf8cc = str2int64("srchash");
	leaf_hash.SetData(source_hash, 20);
	root.AddLeaf(&leaf_hash);

	// key hint
	unsigned char s_key_hint[32] = "test data";
	AESCryptor codec;
	codec.set_key(key, 256);
	codec.encrypt(s_key_hint, s_key_hint+16);
	my12doom_leaf leaf_key_hint;
	leaf_key_hint.m_leaf8cc = str2int64("keyhint");
	leaf_key_hint.SetData(s_key_hint, 32);
	root.AddLeaf(&leaf_key_hint);

	// source hash
	my12doom_leaf leaf_remux;
	char remux_sig[192];
	memset(remux_sig, 0, 192);
	strcpy(remux_sig, "my12doom's mvc interlacer");
	leaf_remux.m_leaf8cc = str2int64("remux");
	leaf_remux.SetData(remux_sig, 192);
	root.AddLeaf(&leaf_remux);

	// write header
	root.WriteTo(hout);

	// copy file with encrypt
	const int E3D_ENCODING_BLOCK_SIZE = 655360;
	unsigned char *tmp = new unsigned char [E3D_ENCODING_BLOCK_SIZE];

	if (cb)
		if (S_OK != cb->CB(0, 0, 0))
			goto clearup;

	for(__int64 byte_left= file_size; byte_left>0; byte_left-= E3D_ENCODING_BLOCK_SIZE)
	{
		DWORD byte_written = 0;
		DWORD byte_read = 0;
		memset(tmp, 0, E3D_ENCODING_BLOCK_SIZE);
		ReadFile(hin, tmp, (DWORD)E3D_ENCODING_BLOCK_SIZE, &byte_read, NULL);
		for(int i=0; i<E3D_ENCODING_BLOCK_SIZE/16; i++)
			codec.encrypt(tmp+i*16, tmp+i*16);
		WriteFile(hout, tmp, E3D_ENCODING_BLOCK_SIZE, &byte_written, NULL);

		if (byte_read < E3D_ENCODING_BLOCK_SIZE)
			break;

		if (cb)
			if (S_OK != cb->CB(1, file_size-byte_left, file_size))
				goto clearup;

	}

	if (cb)
		if (S_OK != cb->CB(2, file_size, 0))
			goto clearup;

clearup:
	delete [] tmp;
	CloseHandle(hin);
	CloseHandle(hout);
	return 0;
}