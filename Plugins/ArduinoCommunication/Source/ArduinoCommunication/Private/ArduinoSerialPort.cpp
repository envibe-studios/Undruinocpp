// Arduino Communication Plugin - Serial Port Implementation

#include "ArduinoSerialPort.h"
#include "Async/Async.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <setupapi.h>
#include "Windows/HideWindowsPlatformTypes.h"
#pragma comment(lib, "setupapi.lib")
#elif PLATFORM_LINUX || PLATFORM_MAC
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <errno.h>
#include <cstring>
#endif

UArduinoSerialPort::UArduinoSerialPort()
	: SerialHandle(nullptr)
	, bIsOpen(false)
	, CurrentBaudRate(115200)
	, ReadThread(nullptr)
	, ReadRunnable(nullptr)
	, bStopThread(false)
{
}

UArduinoSerialPort::~UArduinoSerialPort()
{
	Close();
}

bool UArduinoSerialPort::Open(const FString& PortName, int32 BaudRate)
{
	if (bIsOpen)
	{
		Close();
	}

#if PLATFORM_WINDOWS
	// Format port name for Windows
	FString FormattedPort = PortName;

	// Handle numeric-only input (e.g., "8" -> "COM8")
	if (PortName.IsNumeric())
	{
		FormattedPort = FString::Printf(TEXT("\\\\.\\COM%s"), *PortName);
	}
	else if (!PortName.StartsWith(TEXT("\\\\.\\")))
	{
		FormattedPort = FString::Printf(TEXT("\\\\.\\%s"), *PortName);
	}

	// Open the serial port
	HANDLE hSerial = CreateFileW(
		*FormattedPort,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hSerial == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		FString ErrorDetails;
		switch (error)
		{
			case ERROR_FILE_NOT_FOUND: // 2
				ErrorDetails = TEXT("Port not found. Check that the device is connected and the port name is correct (e.g., COM8).");
				break;
			case ERROR_ACCESS_DENIED: // 5
				ErrorDetails = TEXT("Access denied. The port may be in use by another application.");
				break;
			default:
				ErrorDetails = FString::Printf(TEXT("Error code: %d"), error);
				break;
		}
		FString ErrorMsg = FString::Printf(TEXT("Failed to open port %s. %s"), *FormattedPort, *ErrorDetails);
		UE_LOG(LogTemp, Error, TEXT("ArduinoSerial: %s"), *ErrorMsg);
		OnError.Broadcast(ErrorMsg);
		return false;
	}

	// Configure serial port parameters
	DCB dcbSerialParams = { 0 };
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	if (!GetCommState(hSerial, &dcbSerialParams))
	{
		CloseHandle(hSerial);
		FString ErrorMsg = TEXT("Failed to get serial port state");
		UE_LOG(LogTemp, Error, TEXT("ArduinoSerial: %s"), *ErrorMsg);
		OnError.Broadcast(ErrorMsg);
		return false;
	}

	// Set baud rate
	dcbSerialParams.BaudRate = BaudRate;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
	dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;

	if (!SetCommState(hSerial, &dcbSerialParams))
	{
		CloseHandle(hSerial);
		FString ErrorMsg = TEXT("Failed to set serial port parameters");
		UE_LOG(LogTemp, Error, TEXT("ArduinoSerial: %s"), *ErrorMsg);
		OnError.Broadcast(ErrorMsg);
		return false;
	}

	// Set timeouts
	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = 1;
	timeouts.ReadTotalTimeoutConstant = 10;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;

	if (!SetCommTimeouts(hSerial, &timeouts))
	{
		DWORD error = GetLastError();
		CloseHandle(hSerial);
		FString ErrorMsg = FString::Printf(TEXT("Failed to set serial port timeouts (error=%d)"), error);
		UE_LOG(LogTemp, Error, TEXT("ArduinoSerial: %s"), *ErrorMsg);
		OnError.Broadcast(ErrorMsg);
		return false;
	}

	// Clear any existing data in buffers
	PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

	SerialHandle = hSerial;
	bIsOpen = true;
	CurrentPortName = PortName;
	CurrentBaudRate = BaudRate;

	// Reset raw tap counters for fresh connection
	ResetRawTapCounters();

	UE_LOG(LogTemp, Log, TEXT("ArduinoSerial: Opened port %s at %d baud"), *PortName, BaudRate);

	// Start the read thread (or poll mode)
	StartReadThread();

	// Broadcast connection event
	OnConnectionChanged.Broadcast(true);

	return true;

#elif PLATFORM_LINUX || PLATFORM_MAC
	// Open the serial port
	int fd = open(TCHAR_TO_UTF8(*PortName), O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0)
	{
		FString ErrorMsg = FString::Printf(TEXT("Failed to open port %s. Error: %s"), *PortName, UTF8_TO_TCHAR(strerror(errno)));
		UE_LOG(LogTemp, Error, TEXT("ArduinoSerial: %s"), *ErrorMsg);
		OnError.Broadcast(ErrorMsg);
		return false;
	}

	// Get current port settings
	struct termios tty;
	if (tcgetattr(fd, &tty) != 0)
	{
		close(fd);
		FString ErrorMsg = FString::Printf(TEXT("Failed to get port attributes. Error: %s"), UTF8_TO_TCHAR(strerror(errno)));
		UE_LOG(LogTemp, Error, TEXT("ArduinoSerial: %s"), *ErrorMsg);
		OnError.Broadcast(ErrorMsg);
		return false;
	}

	// Set baud rate
	speed_t BaudRateConstant;
	switch (BaudRate)
	{
		case 9600:   BaudRateConstant = B9600; break;
		case 19200:  BaudRateConstant = B19200; break;
		case 38400:  BaudRateConstant = B38400; break;
		case 57600:  BaudRateConstant = B57600; break;
		case 115200: BaudRateConstant = B115200; break;
		case 230400: BaudRateConstant = B230400; break;
		case 460800: BaudRateConstant = B460800; break;
		default:     BaudRateConstant = B115200; break;
	}
	cfsetispeed(&tty, BaudRateConstant);
	cfsetospeed(&tty, BaudRateConstant);

	// Configure port: 8N1 (8 data bits, no parity, 1 stop bit)
	tty.c_cflag &= ~PARENB;        // No parity
	tty.c_cflag &= ~CSTOPB;        // 1 stop bit
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;            // 8 data bits
	tty.c_cflag &= ~CRTSCTS;       // No hardware flow control
	tty.c_cflag |= CREAD | CLOCAL; // Enable receiver, ignore modem control lines

	// Raw input mode
	tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	// Disable software flow control
	tty.c_iflag &= ~(IXON | IXOFF | IXANY);
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

	// Raw output mode
	tty.c_oflag &= ~OPOST;

	// Non-blocking read with timeout
	tty.c_cc[VMIN] = 0;
	tty.c_cc[VTIME] = 1; // 100ms timeout

	if (tcsetattr(fd, TCSANOW, &tty) != 0)
	{
		close(fd);
		FString ErrorMsg = FString::Printf(TEXT("Failed to set port attributes. Error: %s"), UTF8_TO_TCHAR(strerror(errno)));
		UE_LOG(LogTemp, Error, TEXT("ArduinoSerial: %s"), *ErrorMsg);
		OnError.Broadcast(ErrorMsg);
		return false;
	}

	// Flush any existing data
	tcflush(fd, TCIOFLUSH);

	// Store file descriptor as handle (cast to void*)
	SerialHandle = reinterpret_cast<void*>(static_cast<intptr_t>(fd));
	bIsOpen = true;
	CurrentPortName = PortName;
	CurrentBaudRate = BaudRate;

	// Reset raw tap counters for fresh connection
	ResetRawTapCounters();

	UE_LOG(LogTemp, Log, TEXT("ArduinoSerial: Opened port %s at %d baud"), *PortName, BaudRate);

	// Start the read thread (or poll mode)
	StartReadThread();

	// Broadcast connection event
	OnConnectionChanged.Broadcast(true);

	return true;

#else
	// Unsupported platform
	FString ErrorMsg = TEXT("Serial port not implemented for this platform");
	UE_LOG(LogTemp, Error, TEXT("ArduinoSerial: %s"), *ErrorMsg);
	OnError.Broadcast(ErrorMsg);
	return false;
#endif
}

void UArduinoSerialPort::Close()
{
	if (!bIsOpen)
	{
		return;
	}

	// Stop the read thread first
	StopReadThread();

#if PLATFORM_WINDOWS
	if (SerialHandle != nullptr)
	{
		CloseHandle((HANDLE)SerialHandle);
		SerialHandle = nullptr;
	}
#elif PLATFORM_LINUX || PLATFORM_MAC
	if (SerialHandle != nullptr)
	{
		int fd = static_cast<int>(reinterpret_cast<intptr_t>(SerialHandle));
		close(fd);
		SerialHandle = nullptr;
	}
#endif

	bIsOpen = false;
	ReceiveBuffer.Empty();

	UE_LOG(LogTemp, Log, TEXT("ArduinoSerial: Closed port %s"), *CurrentPortName);

	// Broadcast disconnection event
	OnConnectionChanged.Broadcast(false);
}

bool UArduinoSerialPort::IsOpen() const
{
	return bIsOpen;
}

bool UArduinoSerialPort::SendCommand(const FString& Command)
{
	if (!bIsOpen || SerialHandle == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ArduinoSerial: Cannot send - port not open"));
		return false;
	}

#if PLATFORM_WINDOWS
	// Convert to UTF-8
	FTCHARToUTF8 Converter(*Command);
	const char* Data = Converter.Get();
	int32 DataLength = Converter.Length();

	DWORD bytesWritten = 0;
	BOOL result = WriteFile((HANDLE)SerialHandle, Data, DataLength, &bytesWritten, NULL);

	if (!result || bytesWritten != DataLength)
	{
		FString ErrorMsg = FString::Printf(TEXT("Failed to send data. Wrote %d of %d bytes"), bytesWritten, DataLength);
		UE_LOG(LogTemp, Error, TEXT("ArduinoSerial: %s"), *ErrorMsg);
		OnError.Broadcast(ErrorMsg);
		return false;
	}

	UE_LOG(LogTemp, Verbose, TEXT("ArduinoSerial: Sent: %s"), *Command);
	return true;

#elif PLATFORM_LINUX || PLATFORM_MAC
	int fd = static_cast<int>(reinterpret_cast<intptr_t>(SerialHandle));

	// Convert to UTF-8
	FTCHARToUTF8 Converter(*Command);
	const char* Data = Converter.Get();
	int32 DataLength = Converter.Length();

	ssize_t bytesWritten = write(fd, Data, DataLength);

	if (bytesWritten < 0 || bytesWritten != DataLength)
	{
		FString ErrorMsg = FString::Printf(TEXT("Failed to send data. Wrote %d of %d bytes. Error: %s"),
			(int)bytesWritten, DataLength, UTF8_TO_TCHAR(strerror(errno)));
		UE_LOG(LogTemp, Error, TEXT("ArduinoSerial: %s"), *ErrorMsg);
		OnError.Broadcast(ErrorMsg);
		return false;
	}

	UE_LOG(LogTemp, Verbose, TEXT("ArduinoSerial: Sent: %s"), *Command);
	return true;

#else
	return false;
#endif
}

bool UArduinoSerialPort::SendLine(const FString& Command)
{
	return SendCommand(Command + LineEnding);
}

TArray<FString> UArduinoSerialPort::GetAvailablePorts()
{
	TArray<FString> Ports;

#if PLATFORM_WINDOWS
	// Query available COM ports
	for (int32 i = 1; i <= 256; i++)
	{
		FString PortName = FString::Printf(TEXT("COM%d"), i);
		FString FullPath = FString::Printf(TEXT("\\\\.\\%s"), *PortName);

		HANDLE hPort = CreateFileW(
			*FullPath,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL
		);

		if (hPort != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hPort);
			Ports.Add(PortName);
		}
	}
#elif PLATFORM_LINUX
	// Scan for USB serial devices (/dev/ttyUSB* and /dev/ttyACM*)
	DIR* dir = opendir("/dev");
	if (dir)
	{
		struct dirent* entry;
		while ((entry = readdir(dir)) != nullptr)
		{
			FString DeviceName = UTF8_TO_TCHAR(entry->d_name);
			if (DeviceName.StartsWith(TEXT("ttyUSB")) || DeviceName.StartsWith(TEXT("ttyACM")))
			{
				Ports.Add(FString::Printf(TEXT("/dev/%s"), *DeviceName));
			}
		}
		closedir(dir);
	}
	Ports.Sort();
#elif PLATFORM_MAC
	// Scan for USB serial devices (/dev/tty.usbserial* and /dev/tty.usbmodem*)
	DIR* dir = opendir("/dev");
	if (dir)
	{
		struct dirent* entry;
		while ((entry = readdir(dir)) != nullptr)
		{
			FString DeviceName = UTF8_TO_TCHAR(entry->d_name);
			if (DeviceName.StartsWith(TEXT("tty.usbserial")) || DeviceName.StartsWith(TEXT("tty.usbmodem")))
			{
				Ports.Add(FString::Printf(TEXT("/dev/%s"), *DeviceName));
			}
		}
		closedir(dir);
	}
	Ports.Sort();
#endif

	return Ports;
}

void UArduinoSerialPort::StartReadThread()
{
	bStopThread = false;

	// Set up timer to process received data on game thread
	// Try multiple methods to get a valid world for the timer
	UWorld* World = nullptr;

	// First, try to get world from our outer chain
	if (UObject* Outer = GetOuter())
	{
		World = Outer->GetWorld();
	}

	// Fallback to GEngine's world
	if (!World && GEngine)
	{
		World = GEngine->GetWorld();
	}

	// Last resort: iterate through world contexts
	if (!World && GEngine)
	{
		UWorld* FallbackWorld = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World())
			{
				// Prefer Game or PIE worlds
				if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
				{
					World = Context.World();
					break;
				}
				// Keep track of any valid world as fallback (Editor, etc.)
				if (!FallbackWorld)
				{
					FallbackWorld = Context.World();
				}
			}
		}
		// Use fallback world if no Game/PIE world was found
		if (!World && FallbackWorld)
		{
			World = FallbackWorld;
		}
	}

	if (bUsePollMode)
	{
		// POLL MODE: Read on game thread timer instead of worker thread
		if (World)
		{
			// Poll timer - reads serial data every 10ms on game thread
			World->GetTimerManager().SetTimer(
				PollTimerHandle,
				this,
				&UArduinoSerialPort::PollRead,
				0.010f, // 10ms poll interval
				true
			);

			// Process timer for data/delegates
			World->GetTimerManager().SetTimer(
				ProcessTimerHandle,
				this,
				&UArduinoSerialPort::ProcessReceivedData,
				0.016f, // ~60 FPS
				true
			);
		}
	}
	else
	{
		// THREAD MODE: Read on worker thread (original behavior)
		ReadRunnable = new FSerialReadRunnable(this);
		ReadThread = FRunnableThread::Create(ReadRunnable, TEXT("ArduinoSerialReadThread"));

		if (World)
		{
			World->GetTimerManager().SetTimer(
				ProcessTimerHandle,
				this,
				&UArduinoSerialPort::ProcessReceivedData,
				0.016f, // ~60 FPS
				true
			);
		}
	}

	// Stats timer only runs when verbose diagnostics are enabled
	if (bVerboseDiagnostics && World)
	{
		World->GetTimerManager().SetTimer(
			StatsTimerHandle,
			this,
			&UArduinoSerialPort::LogPeriodicStats,
			1.0f, // Once per second
			true
		);
	}
}

void UArduinoSerialPort::StopReadThread()
{
	bStopThread = true;

	// Clear timer - try multiple methods to get a valid world
	UWorld* World = nullptr;

	if (UObject* Outer = GetOuter())
	{
		World = Outer->GetWorld();
	}

	if (!World && GEngine)
	{
		World = GEngine->GetWorld();
	}

	if (!World && GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World())
			{
				World = Context.World();
				break;
			}
		}
	}

	if (World)
	{
		World->GetTimerManager().ClearTimer(ProcessTimerHandle);
		World->GetTimerManager().ClearTimer(PollTimerHandle);
		World->GetTimerManager().ClearTimer(StatsTimerHandle);
	}

	if (ReadThread != nullptr)
	{
		ReadThread->WaitForCompletion();
		delete ReadThread;
		ReadThread = nullptr;
	}

	if (ReadRunnable != nullptr)
	{
		delete ReadRunnable;
		ReadRunnable = nullptr;
	}
}

FString UArduinoSerialPort::GetRawTapStats() const
{
	return FString::Printf(TEXT("BytesReadTotal=%lld ReadsCount=%lld LastReadSize=%d StartByteHits=%lld ZeroByteReads=%lld LastReadError=%d LastByteTime=%.3f"),
		BytesReadTotal, ReadsCount, LastReadSize, StartByteHits, ZeroByteReads, LastReadError, LastByteTime);
}

void UArduinoSerialPort::ResetRawTapCounters()
{
	FScopeLock Lock(&const_cast<UArduinoSerialPort*>(this)->RawTapCriticalSection);
	BytesReadTotal = 0;
	ReadsCount = 0;
	LastReadSize = 0;
	StartByteHits = 0;
	ZeroByteReads = 0;
	LastReadError = 0;
	LastByteTime = 0.0;
}

void UArduinoSerialPort::SetRawTapOptions(bool bDump, bool bBypass, bool bOnScreen)
{
	bDumpRawSerial = bDump;
	bBypassParser = bBypass;
	bShowRawTapOnScreen = bOnScreen;
}

void UArduinoSerialPort::SetSerialRawTapOptions(bool bDump, bool bBypass, bool bOnScreen, bool bPollMode, bool bVerbose)
{
	bDumpRawSerial = bDump;
	bBypassParser = bBypass;
	bShowRawTapOnScreen = bOnScreen;
	bUsePollMode = bPollMode;
	bVerboseDiagnostics = bVerbose;
}

FString UArduinoSerialPort::FormatHexDump(const uint8* Buffer, int32 BytesRead, int32 MaxBytes)
{
	FString HexStr;
	int32 Count = FMath::Min(BytesRead, MaxBytes);
	for (int32 i = 0; i < Count; ++i)
	{
		HexStr += FString::Printf(TEXT("%02X "), Buffer[i]);
	}
	if (BytesRead > MaxBytes)
	{
		HexStr += TEXT("...");
	}
	return HexStr;
}

FString UArduinoSerialPort::FormatAsciiView(const uint8* Buffer, int32 BytesRead, int32 MaxBytes)
{
	FString AsciiStr;
	int32 Count = FMath::Min(BytesRead, MaxBytes);
	for (int32 i = 0; i < Count; ++i)
	{
		uint8 Byte = Buffer[i];
		// Printable ASCII range: 0x20 (space) to 0x7E (~)
		if (Byte >= 0x20 && Byte <= 0x7E)
		{
			AsciiStr += static_cast<TCHAR>(Byte);
		}
		else
		{
			AsciiStr += TEXT('.');
		}
	}
	return AsciiStr;
}

void UArduinoSerialPort::ProcessRawTap(const uint8* Buffer, int32 BytesRead)
{
	// Update counters (thread-safe)
	int64 LocalReadsCount = 0;
	int64 LocalBytesTotal = 0;
	int64 LocalStartHits = 0;
	{
		FScopeLock Lock(&RawTapCriticalSection);
		BytesReadTotal += BytesRead;
		ReadsCount++;
		LastReadSize = BytesRead;
		LastByteTime = FPlatformTime::Seconds();
		LastReadError = 0; // Successful read

		// Count 0xAA start bytes
		for (int32 i = 0; i < BytesRead; ++i)
		{
			if (Buffer[i] == 0xAA)
			{
				StartByteHits++;
			}
		}

		LocalReadsCount = ReadsCount;
		LocalBytesTotal = BytesReadTotal;
		LocalStartHits = StartByteHits;
	}

	// Log stats only when verbose diagnostics enabled
	if (bVerboseDiagnostics)
	{
		UE_LOG(LogTemp, Log, TEXT("ArduinoRawTap: [READ #%lld] bytesRead=%d bytesTotal=%lld startByteHits=%lld"),
			LocalReadsCount, BytesRead, LocalBytesTotal, LocalStartHits);
	}

	// Hex dump if enabled
	if (bDumpRawSerial)
	{
		FString HexDump = FormatHexDump(Buffer, BytesRead);
		FString AsciiView = FormatAsciiView(Buffer, BytesRead);
		UE_LOG(LogTemp, Log, TEXT("ArduinoRawTap: RAW[%d]: %s"), BytesRead, *HexDump);
		UE_LOG(LogTemp, Log, TEXT("ArduinoRawTap: ASCII: %s"), *AsciiView);
	}

	// On-screen debug display (must execute on game thread)
	// Use a stable key (1001) so it updates in place instead of scrolling
	if (bShowRawTapOnScreen && GEngine)
	{
		FString DebugStr = FString::Printf(TEXT("ArduinoRawTap: bytes=%lld reads=%lld last=%d 0xAA=%lld"),
			LocalBytesTotal, LocalReadsCount, BytesRead, LocalStartHits);

		// Queue for game thread display with stable key
		AsyncTask(ENamedThreads::GameThread, [DebugStr]()
		{
			if (GEngine)
			{
				// Key 1001 = stable position, Duration 2.0 = visible for 2 seconds
				GEngine->AddOnScreenDebugMessage(1001, 2.0f, FColor::Cyan, DebugStr);
			}
		});
	}
}

void UArduinoSerialPort::ProcessReceivedData()
{
	// Process raw bytes
	TArray<uint8> Bytes;
	while (ReceivedBytesQueue.Dequeue(Bytes))
	{
		OnByteReceived.Broadcast(Bytes);
	}

	// Process complete lines
	FString Data;
	while (ReceivedDataQueue.Dequeue(Data))
	{
		OnDataReceived.Broadcast(Data);
		OnLineReceived.Broadcast(Data);
	}

	// Update on-screen debug if enabled (this runs on game thread)
	if (bShowRawTapOnScreen && GEngine)
	{
		FString DebugStr = FString::Printf(TEXT("ArduinoRawTap: bytes=%lld reads=%lld last=%d 0xAA=%lld zeros=%lld err=%d"),
			BytesReadTotal, ReadsCount, LastReadSize, StartByteHits, ZeroByteReads, LastReadError);
		GEngine->AddOnScreenDebugMessage(1001, 0.1f, FColor::Cyan, DebugStr);
	}
}

void UArduinoSerialPort::PollRead()
{
	// POLL MODE: Read serial data on game thread (called from timer)
	if (!bIsOpen || SerialHandle == nullptr)
	{
		return;
	}

	static int64 PollTickCount = 0;
	PollTickCount++;

#if PLATFORM_WINDOWS
	uint8 ReadBuffer[256];
	HANDLE hSerial = (HANDLE)SerialHandle;

	if (hSerial == nullptr || hSerial == INVALID_HANDLE_VALUE)
	{
		return;
	}

	DWORD bytesRead = 0;
	BOOL result = ReadFile(hSerial, ReadBuffer, sizeof(ReadBuffer) - 1, &bytesRead, NULL);
	DWORD lastError = result ? 0 : GetLastError();

	if (result && bytesRead > 0)
	{
		// Process raw tap diagnostics
		ProcessRawTap(ReadBuffer, static_cast<int32>(bytesRead));

		// Enqueue raw bytes for OnByteReceived
		TArray<uint8> RawBytes;
		RawBytes.Append(ReadBuffer, bytesRead);
		ReceivedBytesQueue.Enqueue(RawBytes);

		// If bypass parser mode is enabled, skip all line parsing
		if (!bBypassParser)
		{
			ReadBuffer[bytesRead] = 0;
			FUTF8ToTCHAR Converter(reinterpret_cast<const char*>(ReadBuffer), bytesRead);
			FString ReceivedText(Converter.Length(), Converter.Get());
			ReceiveBuffer += ReceivedText;

			FString Line;
			while (ReceiveBuffer.Split(LineEnding, &Line, &ReceiveBuffer))
			{
				if (!Line.IsEmpty())
				{
					ReceivedDataQueue.Enqueue(Line);
				}
			}
		}
	}
	else if (result && bytesRead == 0)
	{
		FScopeLock Lock(&RawTapCriticalSection);
		ZeroByteReads++;
		ReadsCount++;
	}
	else if (!result)
	{
		FScopeLock Lock(&RawTapCriticalSection);
		LastReadError = static_cast<int32>(lastError);
		ReadsCount++;
	}

#elif PLATFORM_LINUX || PLATFORM_MAC
	uint8 ReadBuffer[256];
	int fd = static_cast<int>(reinterpret_cast<intptr_t>(SerialHandle));

	if (fd < 0)
	{
		return;
	}

	ssize_t bytesRead = read(fd, ReadBuffer, sizeof(ReadBuffer) - 1);
	int lastError = (bytesRead < 0) ? errno : 0;

	if (bytesRead > 0)
	{
		ProcessRawTap(ReadBuffer, static_cast<int32>(bytesRead));

		TArray<uint8> RawBytes;
		RawBytes.Append(ReadBuffer, bytesRead);
		ReceivedBytesQueue.Enqueue(RawBytes);

		if (!bBypassParser)
		{
			ReadBuffer[bytesRead] = 0;
			FUTF8ToTCHAR Converter(reinterpret_cast<const char*>(ReadBuffer), bytesRead);
			FString ReceivedText(Converter.Length(), Converter.Get());
			ReceiveBuffer += ReceivedText;

			FString Line;
			while (ReceiveBuffer.Split(LineEnding, &Line, &ReceiveBuffer))
			{
				if (!Line.IsEmpty())
				{
					ReceivedDataQueue.Enqueue(Line);
				}
			}
		}
	}
	else if (bytesRead == 0)
	{
		FScopeLock Lock(&RawTapCriticalSection);
		ZeroByteReads++;
		ReadsCount++;
	}
	else if (bytesRead < 0 && lastError != EAGAIN && lastError != EWOULDBLOCK)
	{
		FScopeLock Lock(&RawTapCriticalSection);
		LastReadError = lastError;
		ReadsCount++;
	}
#endif
}

void UArduinoSerialPort::UpdateOnScreenDebug()
{
	// Called from game thread timer to update on-screen stats
	if (bShowRawTapOnScreen && GEngine)
	{
		FString DebugStr = FString::Printf(TEXT("ArduinoRawTap: bytes=%lld reads=%lld last=%d 0xAA=%lld zeros=%lld err=%d"),
			BytesReadTotal, ReadsCount, LastReadSize, StartByteHits, ZeroByteReads, LastReadError);
		GEngine->AddOnScreenDebugMessage(1001, 1.1f, FColor::Cyan, DebugStr);
	}
}

void UArduinoSerialPort::LogPeriodicStats()
{
	static int64 StatsTickCount = 0;
	StatsTickCount++;

	double TimeSinceLastByte = (LastByteTime > 0.0) ? (FPlatformTime::Seconds() - LastByteTime) : -1.0;

	UE_LOG(LogTemp, Log, TEXT("ArduinoRawTap: Stats [%lld]: bytesTotal=%lld readsCount=%lld lastReadSize=%d startByteHits=%lld zeroByteReads=%lld lastErr=%d timeSinceLastByte=%.2fs"),
		StatsTickCount,
		BytesReadTotal,
		ReadsCount,
		LastReadSize,
		StartByteHits,
		ZeroByteReads,
		LastReadError,
		TimeSinceLastByte);

	// Update on-screen display if enabled
	if (bShowRawTapOnScreen && GEngine)
	{
		FString DebugStr = FString::Printf(TEXT("RawTap[%lld]: bytes=%lld reads=%lld last=%d 0xAA=%lld zeros=%lld err=%d since=%.1fs"),
			StatsTickCount, BytesReadTotal, ReadsCount, LastReadSize, StartByteHits, ZeroByteReads, LastReadError, TimeSinceLastByte);
		GEngine->AddOnScreenDebugMessage(1001, 1.1f, FColor::Cyan, DebugStr);
	}
}

// FSerialReadRunnable implementation

FSerialReadRunnable::FSerialReadRunnable(UArduinoSerialPort* InOwner)
	: Owner(InOwner)
	, bRunning(false)
{
}

FSerialReadRunnable::~FSerialReadRunnable()
{
}

bool FSerialReadRunnable::Init()
{
	bRunning = true;
	return true;
}

uint32 FSerialReadRunnable::Run()
{
#if PLATFORM_WINDOWS
	// Use uint8 buffer to treat as raw bytes (not char/string)
	uint8 ReadBuffer[256];

	while (bRunning && Owner && Owner->bIsOpen && !Owner->bStopThread)
	{
		HANDLE hSerial = (HANDLE)Owner->SerialHandle;
		if (hSerial == nullptr || hSerial == INVALID_HANDLE_VALUE)
		{
			break;
		}

		DWORD bytesRead = 0;
		BOOL result = ReadFile(hSerial, ReadBuffer, sizeof(ReadBuffer) - 1, &bytesRead, NULL);
		DWORD lastError = result ? 0 : GetLastError();

		if (result && bytesRead > 0)
		{
			// ============================================================
			// RAW TAP - Process raw bytes FIRST (before any conversions)
			// ============================================================
			Owner->ProcessRawTap(ReadBuffer, static_cast<int32>(bytesRead));

			// Enqueue raw bytes for OnByteReceived
			TArray<uint8> RawBytes;
			RawBytes.Append(ReadBuffer, bytesRead);
			Owner->ReceivedBytesQueue.Enqueue(RawBytes);

			// If bypass parser mode is enabled, skip all line parsing
			if (!Owner->bBypassParser)
			{
				// Null-terminate for string conversion (safe - buffer is 256, max read is 255)
				ReadBuffer[bytesRead] = 0;

				// Convert from UTF-8 to FString
				FUTF8ToTCHAR Converter(reinterpret_cast<const char*>(ReadBuffer), bytesRead);
				FString ReceivedText(Converter.Length(), Converter.Get());

				// Add to buffer
				Owner->ReceiveBuffer += ReceivedText;

				// Process complete lines
				FString Line;
				while (Owner->ReceiveBuffer.Split(Owner->LineEnding, &Line, &Owner->ReceiveBuffer))
				{
					if (!Line.IsEmpty())
					{
						Owner->ReceivedDataQueue.Enqueue(Line);
					}
				}
			}
		}
		else if (result && bytesRead == 0)
		{
			// Zero-byte read - track this
			FScopeLock Lock(&Owner->RawTapCriticalSection);
			Owner->ZeroByteReads++;
			Owner->ReadsCount++;
		}
		else if (!result)
		{
			// Read error - track this
			FScopeLock Lock(&Owner->RawTapCriticalSection);
			Owner->LastReadError = static_cast<int32>(lastError);
			Owner->ReadsCount++;
		}

		// Small sleep to prevent busy waiting
		FPlatformProcess::Sleep(0.001f);
	}

#elif PLATFORM_LINUX || PLATFORM_MAC
	// Use uint8 buffer to treat as raw bytes (not char/string)
	uint8 ReadBuffer[256];

	while (bRunning && Owner && Owner->bIsOpen && !Owner->bStopThread)
	{
		int fd = static_cast<int>(reinterpret_cast<intptr_t>(Owner->SerialHandle));
		if (fd < 0)
		{
			break;
		}

		ssize_t bytesRead = read(fd, ReadBuffer, sizeof(ReadBuffer) - 1);
		int lastError = (bytesRead < 0) ? errno : 0;

		if (bytesRead > 0)
		{
			// ============================================================
			// RAW TAP - Process raw bytes FIRST (before any conversions)
			// ============================================================
			Owner->ProcessRawTap(ReadBuffer, static_cast<int32>(bytesRead));

			// Enqueue raw bytes for OnByteReceived
			TArray<uint8> RawBytes;
			RawBytes.Append(ReadBuffer, bytesRead);
			Owner->ReceivedBytesQueue.Enqueue(RawBytes);

			// If bypass parser mode is enabled, skip all line parsing
			if (!Owner->bBypassParser)
			{
				// Null-terminate for string conversion (safe - buffer is 256, max read is 255)
				ReadBuffer[bytesRead] = 0;

				// Convert from UTF-8 to FString
				FUTF8ToTCHAR Converter(reinterpret_cast<const char*>(ReadBuffer), bytesRead);
				FString ReceivedText(Converter.Length(), Converter.Get());

				// Add to buffer
				Owner->ReceiveBuffer += ReceivedText;

				// Process complete lines
				FString Line;
				while (Owner->ReceiveBuffer.Split(Owner->LineEnding, &Line, &Owner->ReceiveBuffer))
				{
					if (!Line.IsEmpty())
					{
						Owner->ReceivedDataQueue.Enqueue(Line);
					}
				}
			}
		}
		else if (bytesRead == 0)
		{
			// Zero-byte read - track this
			FScopeLock Lock(&Owner->RawTapCriticalSection);
			Owner->ZeroByteReads++;
			Owner->ReadsCount++;
		}
		else if (bytesRead < 0 && lastError != EAGAIN && lastError != EWOULDBLOCK)
		{
			// Read error (not just "would block") - track this
			FScopeLock Lock(&Owner->RawTapCriticalSection);
			Owner->LastReadError = lastError;
			Owner->ReadsCount++;
		}

		// Small sleep to prevent busy waiting
		FPlatformProcess::Sleep(0.001f);
	}
#endif

	return 0;
}

void FSerialReadRunnable::Stop()
{
	bRunning = false;
}

void FSerialReadRunnable::Exit()
{
	bRunning = false;
}
