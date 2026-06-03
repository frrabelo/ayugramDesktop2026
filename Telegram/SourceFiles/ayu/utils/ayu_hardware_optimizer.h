// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include <QString>

namespace AyuHardware {

struct HardwareInfo {
	int cpuThreads = 1;
	quint64 totalRamMb = 0;
	QString gpuName = "Unknown GPU";
};

HardwareInfo detectHardware();
int calculateMaxParallelDownloads(const HardwareInfo &info);
int calculateMaxParallelUploads(const HardwareInfo &info);
quint64 calculateMaxCacheBufferBytes(const HardwareInfo &info);

} // namespace AyuHardware
