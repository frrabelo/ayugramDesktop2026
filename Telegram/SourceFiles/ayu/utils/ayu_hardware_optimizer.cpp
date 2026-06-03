// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/utils/ayu_hardware_optimizer.h"

#include <QThread>
#include <QDir>
#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#elif defined(Q_OS_MAC)
#include <sys/types.h>
#include <sys/sysctl.h>
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#include <sys/sysinfo.h>
#endif

namespace AyuHardware {

HardwareInfo detectHardware() {
	HardwareInfo info;

	// 1. CPU Threads
	info.cpuThreads = std::max(1, QThread::idealThreadCount());

	// 2. Memory RAM
#ifdef Q_OS_WIN
	MEMORYSTATUSEX memStatus;
	memStatus.dwLength = sizeof(MEMORYSTATUSEX);
	if (GlobalMemoryStatusEx(&memStatus)) {
		info.totalRamMb = memStatus.ullTotalPhys / (1024 * 1024);
	}
#elif defined(Q_OS_MAC)
	int mib[2] = { CTL_HW, HW_MEMSIZE };
	int64_t physicalMemory = 0;
	size_t length = sizeof(physicalMemory);
	if (sysctl(mib, 2, &physicalMemory, &length, NULL, 0) == 0) {
		info.totalRamMb = physicalMemory / (1024 * 1024);
	}
#elif defined(Q_OS_LINUX)
	struct sysinfo si;
	if (sysinfo(&si) == 0) {
		info.totalRamMb = (si.totalram * si.mem_unit) / (1024 * 1024);
	}
#else
	// Default Fallback: assume 8 GB
	info.totalRamMb = 8192;
#endif

	// 3. GPU Name (Simple Detection)
#ifdef Q_OS_WIN
	DISPLAY_DEVICEW dd;
	dd.cb = sizeof(dd);
	if (EnumDisplayDevicesW(NULL, 0, &dd, 0)) {
		info.gpuName = QString::fromWCharArray(dd.DeviceString);
	}
#elif defined(Q_OS_MAC)
	info.gpuName = "Apple Silicon / Metal GPU";
#else
	info.gpuName = "Generic OpenGL/Vulkan GPU";
#endif

	return info;
}

int calculateMaxParallelDownloads(const HardwareInfo &info) {
	// Dynamically scale based on CPU threads and RAM
	int threads = 2; // base level

	if (info.cpuThreads >= 16 && info.totalRamMb >= 16384) {
		threads = 6;
	} else if (info.cpuThreads >= 8 && info.totalRamMb >= 8192) {
		threads = 4;
	} else if (info.cpuThreads <= 2 || info.totalRamMb <= 3072) {
		threads = 1; // slow hardware
	}

	return threads;
}

int calculateMaxParallelUploads(const HardwareInfo &info) {
	// Telegram MTProto usually performs upload in parts.
	// Uploading in parallel can speed up but must not overload network buffer.
	int uploads = 1;
	if (info.cpuThreads >= 8 && info.totalRamMb >= 8192) {
		uploads = 2;
	}
	return uploads;
}

quint64 calculateMaxCacheBufferBytes(const HardwareInfo &info) {
	// Cache size in memory: 100MB for low end, up to 1GB for high end
	if (info.totalRamMb >= 16384) {
		return 1024ULL * 1024 * 1024; // 1 GB
	} else if (info.totalRamMb >= 8192) {
		return 512ULL * 1024 * 1024;  // 512 MB
	} else {
		return 128ULL * 1024 * 1024;  // 128 MB
	}
}

} // namespace AyuHardware
