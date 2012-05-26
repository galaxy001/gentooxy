/*
 *  Unlocker.cpp
 *
 *  Created by Zenith432 on September 25 2011.
 *  Copyright 2011 Zenith432. All rights reserved.
 *
 *  Permission is hereby granted to use this code as you please.
 */

#ifdef _WIN32
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define PATH_SEP '\\'
#else /* _WIN32 */
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#define PATH_SEP '/'
#endif /* _WIN32 */

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

// #define LOGGING
#define ROT 13U

using std::cout;
using std::cerr;
using std::endl;
using std::string;

namespace {

char const random[] = "bheuneqjbexolgurfrjbeqfthneqrqcyrnfrqbagfgrny(p)NccyrPbzchgreVap";
int uninstall = 0;
#ifdef __ESXi__
struct _esxi_t {
	int fd;
	void* points[3];
	size_t sizes[3];
} esxi;
#endif /* __ESXi__ */

#ifdef _WIN32
char const vmx[] = "vmware-vmx.exe";
char const vmx_debug[] = "vmware-vmx-debug.exe";
char const vmx_stats[] = "vmware-vmx-stats.exe";
char const vmwarebase[] = "vmwarebase.dll";
char const subKey1[] = "SOFTWARE\\VMware, Inc.\\VMware Workstation";
char const subKey2[] = "SOFTWARE\\VMware, Inc.\\VMware Player";
char const value1[] = "InstallPath";
char const value2[] = "InstallPath64";
#endif /* _WIN32 */

#ifdef __APPLE__
char const vmx[] = "vmware-vmx";
char const vmx_debug[] = "vmware-vmx-debug";
char const vmx_stats[] = "vmware-vmx-stats";
char const vmwarebase[] = "libvmwarebase.dylib";
char const install_path[] = "/Applications/VMware Fusion.app/Contents/";
char const vmx_path[] = "Library/";
char const vmwarebase_path[] = "Frameworks/";
#endif /* __APPLE__ */

#ifdef __ESXi__
char const vmx[] = "vmx";
char const vmx_debug[] = "vmx-debug";
char const vmx_stats[] = "vmx-stats";
char const install_path[] = "/bin/";
#elif defined(__linux__)
char const vmx[] = "vmware-vmx";
char const vmx_debug[] = "vmware-vmx-debug";
char const vmx_stats[] = "vmware-vmx-stats";
char const vmwarebase[] = "libvmwarebase.so.0";
char const install_path[] = "/opt/vmware/lib/vmware/";
char const vmx_path[] = "bin/";
char const vmwarebase_path[] = "lib/libvmwarebase.so.0/";
#endif /* __ESXi__ || __linux__ */

void rot13(void* base, size_t length)
{
	unsigned char c, e, *p, *q;

	p = static_cast<unsigned char*>(base);
	q = p + length;
	for (; p != q; ++p) {
		c = *p;
		if (c >='A' && c <='Z') {
			if ((e = c + ROT) <= 'Z')
				*p = e;
			else
				*p = c - ROT;
		} else if (c >='a' && c <='z') {
			if ((e = c + ROT) <= 'z')
				*p = e;
			else
				*p = c - ROT;
		}
	}
}

void osk0(char* tail)
{
#ifdef LOGGING
	cout << "Found OSK0 @ " << static_cast<void*>(tail) << endl;
#endif /* LOGGING */
	if (*tail)
		return;
#ifdef __ESXi__
	esxi.points[0] = &tail[1];
	esxi.sizes[0] = 32U;
#endif /* __ESXi__ */
	if (uninstall) {
		memset(&tail[1], 0, 32U);
		return;
	}
	memcpy(&tail[1], &random[0], 32U);
	rot13(&tail[1], 32U);
}

void osk1(char* tail)
{
#ifdef LOGGING
	cout << "Found OSK1 @ " << static_cast<void*>(tail) << endl;
#endif /* LOGGING */
	if (*tail)
		return;
#ifdef __ESXi__
	esxi.points[1] = &tail[1];
	esxi.sizes[1] = 32U;
#endif /* __ESXi__ */
	if (uninstall) {
		memset(&tail[1], 0, 32U);
		return;
	}
	memcpy(&tail[1], &random[32], 32U);
	rot13(&tail[1], 32U);
}

void srvr(char* tail)
{
#ifdef LOGGING
	cout << "Found SRVR @ " << static_cast<void*>(tail) << endl;
#endif /* LOGGING */
#ifdef __ESXi__
	esxi.points[2] = &tail[-5];
	esxi.sizes[2] = 7U;
#endif /* __ESXi__ */
	if (uninstall) {
		tail[-5] = 1;
		tail[1] = 4;
		return;
	}
	if (tail[-5] == 1)
		tail[-5] = 0;
	if (tail[1] == 4)
		tail[1] = 0;
}

void patch_vmx_block(void* base, size_t length)
{
	static char const table[4][9] =
	{
		{ 'O', 'S', 'K', '0', 0, 0, 'R', 'V', 'R' },
		{  1 ,  2 ,  3 ,  4 , 0, 0,  7 ,  8 ,  9  },
		{ 'S',  0 , 'R', '1', 0, 0, 0, 0, 0 },
		{  6 ,  0 ,  7 ,  5 , 0, 0, 0, 0, 0 }
	};
	char c, s, *p, *q;

	p = static_cast<char*>(base);
	q = p + length;
	s = 0;
	while (p != q) {
		c = *p;
		if (c == table[0][s]) {
			s = table[1][s];
			++p;
		} else if (c == table[2][s]) {
			s = table[3][s];
			++p;
		} else if (s) {
			s = 0;
			continue;
		} else {
			++p;
			continue;
		}
		switch (s) {
		case 4:
			osk0(p);
			break;
		case 5:
			osk1(p);
			break;
		case 9:
			srvr(p);
			s = 0;
			break;
		}
	}
}

#ifdef _WIN32
void patch_vmwarebase(void* base, size_t length)
{
	static unsigned char const thumbprint[] =
	{
		0x33, 0xC9, 0x0B, 0xC1, 0x75, 0x09, 0xC6, 0x85, 0xD0, 0xFE, 0xFF, 0xFF, 0x01, 0xEB, 0x07, 0xC6,
		0x85, 0xD0, 0xFE, 0xFF, 0xFF, 0x00, 0x8B, 0x85,	0xD0, 0xFE, 0xFF, 0xFF, 0x50
	};
	unsigned char *p, *q;
	size_t l;

	p = static_cast<unsigned char*>(base);
	q = p + length;
	l = 0U;
	while (p != q) {
		if (l == (sizeof thumbprint - 8U)) {
			++p; ++l;	// skip patch byte
		} else if (*p == thumbprint[l]) {
			++p; ++l;
			if (l == sizeof thumbprint) {
				/*
				 * Eureka
				 */
				p[-8] = uninstall ? 0U : 1U;
				return;
			}
		} else if (l)
			l = 0U;
		else
			*p++;
	}
	cerr << "Pattern not found in " << &vmwarebase[0] << endl;
}

char const* win32_strerror(DWORD code)
{
	static char buffer[256];
	DWORD rc;

	rc = FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		&buffer[0],
		sizeof buffer,
		NULL);
	if (!rc)
		_snprintf(&buffer[0], sizeof buffer, "%u", code);
	return &buffer[0];
}

int map_file(char const* file_name, void** base, size_t* length)
{
	HANDLE fh, mh;
	LARGE_INTEGER l;
	void* p;

	if (!file_name || !base || !length)
		return 0;
	fh = CreateFile(file_name,
		FILE_GENERIC_READ | FILE_GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0U,
		NULL);
	if (fh == INVALID_HANDLE_VALUE) {
		cerr << "CreateFile failed " << win32_strerror(GetLastError()) << endl;
		return 0;
	}
	if (!GetFileSizeEx(fh, &l)) {
		cerr << "GetFileSizeEx failed " << win32_strerror(GetLastError()) << endl;
		CloseHandle(fh);
		return 0;
	}
	if (l.HighPart) {
		cerr << "File Too Big" << endl;
		CloseHandle(fh);
		return 0;
	}
	mh = CreateFileMapping(
		fh,
		NULL,
		PAGE_READWRITE,
		0U,
		0U,
		NULL);
	if (mh == NULL) {
		cerr << "CreateFileMapping failed " << win32_strerror(GetLastError()) << endl;
		CloseHandle(fh);
		return 0;
	}
	CloseHandle(fh);
	p = MapViewOfFile(
		mh,
		FILE_MAP_ALL_ACCESS,
		0U,
		0U,
		0U);
	if (!p) {
		cerr << "MapViewOfFile failed " << win32_strerror(GetLastError()) << endl;
		CloseHandle(mh);
		return 0;
	}
	CloseHandle(mh);
	*base = p;
	*length = l.LowPart;
	return 1;
}

void unmap_file(void* base, size_t length)
{
	if (!base)
		return;
	if (!UnmapViewOfFile(base))
		cerr << "UnmapViewOfFile failed " << win32_strerror(GetLastError()) << endl;
}

int try_get_path(HKEY kh, char const* valueName, string& target, int fatal)
{
	static char buffer[MAX_PATH];
	LONG rc;
	DWORD c, t;

	c = sizeof buffer;
	rc = RegQueryValueEx(
		kh,
		valueName,
		0U,
		&t,
		reinterpret_cast<LPBYTE>(&buffer[0]),
		&c);
	if (rc != ERROR_SUCCESS) {
		if (fatal)
			cerr << "RegQueryValueEx[" << valueName << "] failed " << win32_strerror(rc) << endl;
		return 0;
	}
	if (t != REG_SZ) {
		if (fatal)
			cerr << "RegQueryValueEx[" << valueName << "] not REG_SZ" << endl;
		return 0;
	}
	if (c && buffer[c - 1U] == '\0')
		--c;
	target.reserve(c + 1U);
	target.assign(&buffer[0], c);
	if (c && buffer[c - 1U] != PATH_SEP)
		target.append(1U, PATH_SEP);
	return 1;
}

int get_ipaths(string& vmxPath, string& vmxPath64, string& vmwarebasePath)
{
	LONG rc;
	HKEY kh;

	rc = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		&subKey1[0],
		0U,
		KEY_READ | KEY_WOW64_32KEY,
		&kh);
	if (rc != ERROR_SUCCESS) {
		rc = RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			&subKey2[0],
			0U,
			KEY_READ | KEY_WOW64_32KEY,
			&kh);
		if (rc != ERROR_SUCCESS) {
			cerr << "RegOpenKeyEx failed " << win32_strerror(rc) << endl;
			return 0;
		}
	}
	if (!try_get_path(kh, &value1[0], vmxPath, 1)) {
		RegCloseKey(kh);
		return 0;
	}
	vmwarebasePath.assign(vmxPath);
	if (!try_get_path(kh, &value2[0], vmxPath64, 0))
		cout << "Detected 32-bit Installation" << endl;
	RegCloseKey(kh);
	return 1;
}
#elif defined(__ESXi__)
void patch_vmwarebase(void* base, size_t length)
{
}

int map_file(char const* file_name, void** base, size_t* length)
{
	int fd, rc;
	ssize_t n;
	struct stat stat_buf;
	void* p;

	if (!file_name || !base || !length)
		return 0;
	fd = open(file_name, O_RDWR);
	if (fd < 0) {
		cerr << "open failed " << strerror(errno) << endl;
		return 0;
	}
	if (fstat(fd, &stat_buf) < 0) {
		cerr << "fstat failed " << strerror(errno) << endl;
		close(fd);
		return 0;
	}
	p = malloc(stat_buf.st_size);
	if (!p) {
		cerr << "malloc failed" << endl;
		close(fd);
		return 0;
	}
	n = read(fd, p, stat_buf.st_size);
	if (n < 0) {
		cerr << "read failed " << strerror(errno) << endl;
		free(p);
		close(fd);
		return 0;
	}
	if (n != stat_buf.st_size) {
		cerr << "read short (" << n << '/' << stat_buf.st_size << ')' << endl;
		free(p);
		close(fd);
		return 0;
	}
	memset(&esxi, 0, sizeof esxi);
	esxi.fd = fd;
	*base = p;
	*length = static_cast<size_t>(stat_buf.st_size);
	return 1;
}

void unmap_file(void* base, size_t length)
{
	int i;
	long u, v;

	if (!base)
		return;
	for (i = 0; i != 3; ++i) {
		if (!esxi.points[i] || !esxi.sizes[i])
			continue;
		v = reinterpret_cast<off_t>(esxi.points[i]) - reinterpret_cast<off_t>(base);
		u = lseek(esxi.fd, v, SEEK_SET);
		if (u < 0) {
			cerr << "lseek failed " << strerror(errno) << endl;
			break;
		}
		if (u != v) {
			cerr << "lseek wrong position (" << u << "!=" << v << ')' << endl;
			break;
		}
		u = write(esxi.fd, esxi.points[i], esxi.sizes[i]);
		if (u < 0) {
			cerr << "write failed " << strerror(errno) << endl;
			break;
		}
		if (u != static_cast<long>(esxi.sizes[i])) {
			cerr << "write short (" << u << '/' << esxi.sizes[i] << ')' << endl;
			break;
		}
	}
	free(base);
	close(esxi.fd);
}

int get_ipaths(string& vmxPath, string& vmxPath64, string& vmwarebasePath)
{
	vmxPath.reserve(sizeof install_path - 1U);
	vmxPath.assign(&install_path[0], sizeof install_path - 1U);
	return 1;
}
#else /* _WIN32 || __ESXi__ */
void patch_vmwarebase(void* base, size_t length)
{
	static unsigned char const thumbprint[] =
	{
#ifdef __APPLE__
		0x41, 0xF6, 0x45, 0xF8, 0x01, 0x0F, 0x85, 0x15, 0x03, 0x00, 0x00, 0x31, 0xD2
#elif defined(_LP64)
		0x28, 0xF6, 0x42, 0xF8, 0x01, 0x0F, 0x85, 0x02, 0x03, 0x00, 0x00, 0x31, 0xD2
#else /* Linux 32-bit */
		0xFF, 0xF6, 0x40, 0xF8, 0x01, 0x0F, 0x85, 0xF4, 0x04, 0x00, 0x00, 0x31, 0xC0
#endif
	};
	static unsigned char const patched[] =
	{
#if defined(__APPLE__) || defined(_LP64)
		0xBA,
#else /* Linux 32-bit */
		0xB8,
#endif
			  0x01, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90
	};
	unsigned char *p, *q;
	size_t l;
	size_t const patch_point = 5U;

	p = static_cast<unsigned char*>(base);
	q = p + length;
	l = 0U;
	while (p != q) {
		if (uninstall && l == patch_point) {
			// skip patch bytes
			if (memcmp(p, &patched[0], sizeof patched)) {
				l = 0U;
				continue;
			}
			l += sizeof patched;
			p += sizeof patched;
			goto check_match;
		} else if (*p == thumbprint[l]) {
			++p; ++l;
check_match:
			if (l == sizeof thumbprint) {
				/*
				 * Eureka
				 */
				memcpy(p - sizeof thumbprint + patch_point,
					   uninstall ? &thumbprint[patch_point] : &patched[0],
					   sizeof patched);
				return;
			}
		}
		else if (l)
			l = 0U;
		else
			*p++;
	}
	cerr << "Pattern not found in " << &vmwarebase[0] << endl;
}

int map_file(char const* file_name, void** base, size_t* length)
{
	int fd, rc;
	struct stat stat_buf;
	void* p;

	if (!file_name || !base || !length)
		return 0;
	fd = open(file_name, O_RDWR);
	if (fd < 0) {
		cerr << "open failed " << strerror(errno) << endl;
		return 0;
	}
	if (fstat(fd, &stat_buf) < 0) {
		cerr << "fstat failed " << strerror(errno) << endl;
		close(fd);
		return 0;
	}
	p = mmap(0, static_cast<size_t>(stat_buf.st_size), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		cerr << "mmap failed " << strerror(errno) << endl;
		close(fd);
		return 0;
	}
	close(fd);
	*base = p;
	*length = static_cast<size_t>(stat_buf.st_size);
	return 1;
}

void unmap_file(void* base, size_t length)
{
	if (!base)
		return;
	if (munmap(base, length) < 0)
		cerr << "munmap failed " << strerror(errno) << endl;
}

int get_ipaths(string& vmxPath, string& vmxPath64, string& vmwarebasePath)
{
	vmxPath.reserve(sizeof install_path + sizeof vmx_path - 2U);
	vmxPath.assign(&install_path[0], sizeof install_path - 1U);
	vmxPath.append(&vmx_path[0], sizeof vmx_path - 1U);
	vmwarebasePath.reserve(sizeof install_path + sizeof vmwarebase_path - 2U);
	vmwarebasePath.assign(&install_path[0], sizeof install_path - 1U);
	vmwarebasePath.append(&vmwarebase_path[0], sizeof vmwarebase_path - 1U);
	return 1;
}
#endif /* _WIN32 || __ESXi__ */

int patch_one(string const& name, int kind)
{
	int rc;
	void* base;
	size_t length;

	cout << "Patching " << name << endl;
	rc = map_file(name.c_str(), &base, &length);
	if (!rc)
		return 0;
#ifdef LOGGING
	cout << "File mapped @" << base << " length " << length << endl;
#endif /* LOGGING */
	if (kind)
		patch_vmx_block(base, length);
	else
		patch_vmwarebase(base, length);
	unmap_file(base, length);
	return 1;
}

void usage(char const* self)
{
	cerr << "Usage: " << self << " [-h] [-u] [target_directory]" << endl;
	cerr << "  -h: print help" << endl;
	cerr << "  -u: remove the patch" << endl;
	cerr << "  target_directory: customize location of vmx executables" << endl;
}

int process_options(int argc, char* argv[], string& target_path, int* puninstall)
{
	int i, rc;
	char* s;
	size_t l;

	for (rc = 0, i = 1; i < argc; ++i) {
		s = argv[i];
		if (!s)
			continue;
		if (*s == '-') {
			++s;
			if (strchr(s, 'h')) {
				usage(argc > 0 ? argv[0] : "Unlocker");
				return -1;
			}
			if (puninstall && strchr(s, 'u'))
				*puninstall = 1;
			continue;
		}
		if (rc)
			continue;
		l = strlen(s);
		if (!l)
			continue;
		target_path.reserve(l + 1U);
		target_path.assign(s, l);
		if (s[l - 1U] != PATH_SEP)
			target_path.append(1U, PATH_SEP);
		rc = 1;
	}
	return rc;
}

}

int
#ifdef _WIN32
__cdecl
#endif /* _WIN32 */
main(int argc, char* argv[])
{
	int rc;
	string workPath, vmxPath, vmxPath64, vmwarebasePath;

	rc = process_options(argc, argv, vmxPath, &uninstall);
	if (rc < 0)
		return 0;
#ifndef _WIN32
	if (geteuid() != 0) {
		cerr << "This program must be run as root" << endl;
		return 1;
	}
#endif /* _WIN32 */
	if (uninstall)
		cout << "Removing patches and restoring original form" << endl;
	if (rc)
		vmwarebasePath.assign(vmxPath);
	else {
		rc = get_ipaths(vmxPath, vmxPath64, vmwarebasePath);
		if (!rc)
			return 1;
	}
	workPath.assign(vmxPath);
	workPath.append(&vmx[0], sizeof vmx - 1U);
	rc = patch_one(workPath, 1);
	if (!rc)
		return 1;
	workPath.assign(vmxPath);
	workPath.append(&vmx_debug[0], sizeof vmx_debug - 1U);
	patch_one(workPath, 1);	// ignore error
	workPath.assign(vmxPath);
	workPath.append(&vmx_stats[0], sizeof vmx_stats - 1U);
	patch_one(workPath, 1);	// ignore error
#ifndef __ESXi__
	workPath.assign(vmwarebasePath);
	workPath.append(&vmwarebase[0], sizeof vmwarebase - 1U);
	patch_one(workPath, 0);	// ignore error
#endif /* __ESXi__ */
#ifdef _WIN32
	if (vmxPath64.empty())
		return 0;
	workPath.assign(vmxPath64);
	workPath.append(&vmx[0], sizeof vmx - 1U);
	patch_one(workPath, 1);	// ignore error
	workPath.assign(vmxPath64);
	workPath.append(&vmx_debug[0], sizeof vmx_debug - 1U);
	patch_one(workPath, 1);	// ignore error
	workPath.assign(vmxPath64);
	workPath.append(&vmx_stats[0], sizeof vmx_stats - 1U);
	patch_one(workPath, 1);	// ignore error
#endif /* _WIN32 */
	return 0;
}
