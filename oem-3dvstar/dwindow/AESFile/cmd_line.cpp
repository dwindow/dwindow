#include <tchar.h>
#include <stdio.h>
#include <Windows.h>
#include <WinSock.h>
#include <time.h>
#include <InitGuid.h>

#include "rijndael.h"
#include "E3DReader.h"
#include "E3DWriter.h"
#include "mysql.h"
#include "..\libchecksum\libchecksum.h"

#pragma  comment(lib, "libmySQL.lib")

class dummy_CB : public E3D_Writer_Progress
{
private:
	HRESULT CB(int type, __int64 para1, __int64 para2)
	{
		if (type == 0)
		{
			printf("head written, encrypting started.\n");
		}
		else if (type == 1)
		{
			printf("\r%lld / %lld, %.1f%%", para1, para2, (double)(para1) * 100 / para2);
		}
		else if (type == 2)
		{
			printf("\r%lld / %lld, %.1f%%", para1, para1, (double)(para1) * 100 / para1);
		}

		return S_OK;
	}
} cb;

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc<2)
		return 0;

	if (argc == 4 && argv[3][0] == L'd')
	{
		file_reader reader;
		reader.SetFile(CreateFileW (argv[1], GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));

		if (!reader.m_is_encrypted)
		{
			printf("not a E3D encrypted file.\n");
			return 0;
		}

		// try key file
		unsigned char key[36] = "Hello World!";
		wchar_t keyfile[MAX_PATH];
		wcscpy(keyfile, argv[1]);
		wcscat(keyfile, L".key");
		FILE * key_f = _wfopen(keyfile, L"rb");
		if (key_f)
		{
			fread(key, 1, 32, key_f);
			fclose(key_f);
		}
		reader.set_key(key);
		if (reader.m_key_ok)
		{
			printf("key file OK!.\n");
			goto decrypting;
		}
		else
		{
			printf("key file failed, trying get key from database.\n");
		}		

		//database
		MYSQL mysql;
		mysql_init(&mysql);
		if (mysql_real_connect(&mysql, "127.0.0.1", "root", "tester88", "mydb",3306, NULL, 0))
		{
			printf("db connected.\n");

			// hash2string
			char str_key[65] = "";
			char str_hash[41] = "";
			char tmp[3] = "";
			for(int i=0; i<20; i++)
			{
				sprintf(tmp, "%02X", reader.m_hash[i]);
				strcat(str_hash, tmp);
			}

			// search for existing record
			printf("searching for file %s.\n", str_hash);
			char select_command[1024];
			sprintf(select_command, "select * from movies where hash=\'%s\';", str_hash);

			if(mysql_real_query(&mysql, select_command, (UINT)strlen(select_command)))
			{
				printf("db error.\n");
				exit (-1);
			}

			MYSQL_RES *select_result = mysql_use_result(&mysql);

			int record_found = 0;
			MYSQL_ROW row;
			while (row = mysql_fetch_row(select_result))
			{
				printf("found: record %s,  pass = %s.\n", row[0], row[2]);
				printf("using this key.\n");
				strcpy(str_key, row[2]);
				for(int i=0; i<32; i++)
					sscanf(str_key+2*i, "%02X", key+i);
				record_found++;

				printf("testing key %s....", str_key);
				reader.set_key(key);
				printf(reader.m_key_ok ? "OK!\n" : "FAIL.\n");
				if (reader.m_key_ok)
					break;
			}
			mysql_free_result(select_result);

			if (record_found  == 0)
			{
				printf("no record found in dababase, exiting.\n");
			}
		}

		else
		{
			printf("db fail.\n");
		}
		mysql_close(&mysql);

decrypting:
		// decrypting
		if (!reader.m_key_ok)
			return -1;
		printf("start decrypting..\n");
		FILE *output = _wfopen(argv[2], L"wb");
		const int read_size = 655360;
		char *tmp = new char[read_size];
		LARGE_INTEGER size;
		reader.GetFileSizeEx(&size);
		for(__int64 byte_left= size.QuadPart; byte_left>0; byte_left-= read_size)
		{
			DWORD byte_read = 0;
			reader.ReadFile(tmp, (DWORD)min(byte_left, read_size), &byte_read, NULL);
			fwrite(tmp, byte_read, 1, output);

			if (byte_read < read_size)
				break;

			printf("\r%lld / %lld, %.1f%%", size.QuadPart-byte_left, size.QuadPart, (double)(size.QuadPart-byte_left) * 100 / size.QuadPart);
		}

		delete [] tmp;
		printf("\r%lld/ %lld\n", size.QuadPart, size.QuadPart);
		printf("OK.\n");
		fclose(output);
	}
	else if (argc == 3)
	{
		// hash
		unsigned char hash[20];
		video_checksum(argv[1], (DWORD*)hash);

		char str_hash[41] = "";
		char tmp[3] = "";
		for(int i=0; i<20; i++)
		{
			sprintf(tmp, "%02X", hash[i]);
			strcat(str_hash, tmp);
		}

		// key
		unsigned char key[36] = "The World! testing";		// with a int NULL terminator
		char str_key[65] = "";
		srand(time(NULL));
		for(int i=0; i<32; i++)
		{
			key[i] = rand()%256;
			sprintf(tmp, "%02X", key[i]);
			strcat(str_key, tmp);
		}


		// database
		MYSQL mysql;
		mysql_init(&mysql);
		if (mysql_real_connect(&mysql, "127.0.0.1", "root", "tester88", "mydb",3306, NULL, 0))
		{
			printf("db connected.\n");

			// search for existing record
			printf("searching for file %s.\n", str_hash);
			char select_command[1024];
			sprintf(select_command, "select * from movies where hash=\'%s\';", str_hash);

			if(mysql_real_query(&mysql, select_command, (UINT)strlen(select_command)))
			{
				printf("db error.\n");
				exit (-1);
			}
			
			MYSQL_RES *select_result = mysql_use_result(&mysql);

			int record_found = 0;
			MYSQL_ROW row;
			while (row = mysql_fetch_row(select_result))
			{
				printf("found: record %s,  pass = %s.\n", row[0], row[2]);
				printf("using this key.\n");
				strcpy(str_key, row[2]);
				for(int i=0; i<32; i++)
					sscanf(str_key+2*i, "%02X", key+i);
				record_found++;
			}
			mysql_free_result(select_result);

			if (record_found  == 0)
			{
				printf("no record found, generated new pass = %s.\n", str_key);
			}

			// encoding
			printf("encoding file %s with key = %s.\n", str_hash, str_key);
			WCHAR key_file[MAX_PATH];
			wcscpy(key_file, argv[2]);
			wcscat(key_file, L".key");
			FILE * f = _wfopen(key_file, L"wb");
			if (f){fwrite(key, 1, 32, f); fclose(f);}
			encode_file(argv[1], argv[2], key, hash, &cb);

			if (record_found == 0)
			{
				char insert_command[1024];
				sprintf(insert_command, "insert into movies (hash, pass) values (\'%s\', \'%s\');", str_hash, str_key);
				printf("inserting to database....");
				if (mysql_real_query(&mysql, insert_command, (UINT)strlen(insert_command)))
				{
					printf("DB ERROR : %s.\n", mysql_error(&mysql));
				}
				else
				{
					printf("OK.\n");
				}
			}
		}

		else
		{
			printf("db fail.\n");
		}
		mysql_close(&mysql);
	}






	else if (argc == 2)
	{
		// hash
		file_reader reader;
		HANDLE h_file = CreateFileW (argv[1], GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		reader.SetFile(h_file);
		CloseHandle(h_file);

		if (!reader.m_is_encrypted)
		{
			printf("not a E3D encrypted file, exiting.\n");
			return -1;
		}

		char str_hash[41] = "";
		char tmp[3] = "";
		for(int i=0; i<20; i++)
		{
			sprintf(tmp, "%02X", reader.m_hash[i]);
			strcat(str_hash, tmp);
		}

		// key
		unsigned char key[36] = "";		// with a int NULL terminator
		char str_key[65] = "";
		for(int i=0; i<32; i++)
		{
			key[i] = rand()%256;
			sprintf(tmp, "%02X", key[i]);
			strcat(str_key, tmp);
		}

		wchar_t file_key[MAX_PATH];
		wcscpy(file_key, argv[1]);
		wcscat(file_key, L".key");
		FILE * f = _wfopen(file_key, L"rb");
		if (!f)
		{
			printf("Key file not found, exiting.\n");
			return -1;
		}
		fread(key, 1, 32, f);
		fclose(f);
		reader.set_key(key);
		if (!reader.m_key_ok)
		{
			printf("Key file didn't match, exiting.\n");
			return -1;
		}



		// database
		MYSQL mysql;
		mysql_init(&mysql);
		if (mysql_real_connect(&mysql, "127.0.0.1", "root", "tester88", "mydb",3306, NULL, 0))
		{
			printf("db connected.\n");

			// search for existing record
			printf("searching for file %s.\n", str_hash);
			char select_command[1024];
			sprintf(select_command, "select * from movies where hash=\'%s\';", str_hash);

			if(mysql_real_query(&mysql, select_command, (UINT)strlen(select_command)))
			{
				printf("db error.\n");
				return -1;
			}

			MYSQL_RES *select_result = mysql_use_result(&mysql);

			int record_found = 0;
			MYSQL_ROW row;
			while (row = mysql_fetch_row(select_result))
			{
				printf("found: record %s,  pass = %s.\n", row[0], row[2]);
				strcpy(str_key, row[2]);
				for(int i=0; i<32; i++)
					sscanf(str_key+2*i, "%02X", key+i);
				reader.set_key(key);
				printf(reader.m_key_ok ? "key match.\n":"key didn't match.\n");
				record_found++;
			}
			mysql_free_result(select_result);


			if (record_found == 0 || !reader.m_key_ok)
			{
				char insert_command[1024];
				sprintf(insert_command, "insert into movies (hash, pass) values (\'%s\', \'%s\');", str_hash, str_key);
				printf("inserting to database....");
				if (mysql_real_query(&mysql, insert_command, (UINT)strlen(insert_command)))
				{
					printf("DB ERROR : %s.\n", mysql_error(&mysql));
				}
				else
				{
					printf("OK.\n");
				}
			}
		}

		else
		{
			printf("db fail.\n");
		}
		mysql_close(&mysql);
	}
	return 0;
}