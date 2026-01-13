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
	if (!PortName.StartsWith(TEXT("\\\\.\\")))
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
		FString ErrorMsg = FString::Printf(TEXT("Failed to open port %s. Error code: %d"), *PortName, error);
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
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;

	if (!SetCommTimeouts(hSerial, &timeouts))
	{
		CloseHandle(hSerial);
		FString ErrorMsg = TEXT("Failed to set serial port timeouts");
		UE_LOG(LogTemp, Error, TEXT("ArduinoSerial: %s"), *ErrorMsg);
		OnError.Broadcast(ErrorMsg);
		return false;
	}

	// Clear any existing data
	PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

	SerialHandle = hSerial;
	bIsOpen = true;
	CurrentPortName = PortName;
	CurrentBaudRate = BaudRate;

	UE_LOG(LogTemp, Log, TEXT("ArduinoSerial: Opened port %s at %d baud"), *PortName, BaudRate);

	// Start the read thread
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

	UE_LOG(LogTemp, Log, TEXT("ArduinoSerial: Opened port %s at %d baud"), *PortName, BaudRate);

	// Start the read thread
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
	ReadRunnable = new FSerialReadRunnable(this);
	ReadThread = FRunnableThread::Create(ReadRunnable, TEXT("ArduinoSerialReadThread"));

	// Set up timer to process received data on game thread
	if (GEngine && GEngine->GetWorld())
	{
		GEngine->GetWorld()->GetTimerManager().SetTimer(
			ProcessTimerHandle,
			this,
			&UArduinoSerialPort::ProcessReceivedData,
			0.016f, // ~60 FPS
			true
		);
	}
}

void UArduinoSerialPort::StopReadThread()
{
	bStopThread = true;

	// Clear timer
	if (GEngine && GEngine->GetWorld())
	{
		GEngine->GetWorld()->GetTimerManager().ClearTimer(ProcessTimerHandle);
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

void UArduinoSerialPort::ProcessReceivedData()
{
	FString Data;
	while (ReceivedDataQueue.Dequeue(Data))
	{
		OnDataReceived.Broadcast(Data);
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
	char ReadBuffer[256];

	while (bRunning && Owner && Owner->bIsOpen && !Owner->bStopThread)
	{
		HANDLE hSerial = (HANDLE)Owner->SerialHandle;
		if (hSerial == nullptr || hSerial == INVALID_HANDLE_VALUE)
		{
			break;
		}

		DWORD bytesRead = 0;
		BOOL result = ReadFile(hSerial, ReadBuffer, sizeof(ReadBuffer) - 1, &bytesRead, NULL);

		if (result && bytesRead > 0)
		{
			ReadBuffer[bytesRead] = '\0';

			// Convert from UTF-8 to FString
			FUTF8ToTCHAR Converter(ReadBuffer, bytesRead);
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

		// Small sleep to prevent busy waiting
		FPlatformProcess::Sleep(0.001f);
	}
#elif PLATFORM_LINUX || PLATFORM_MAC
	char ReadBuffer[256];

	while (bRunning && Owner && Owner->bIsOpen && !Owner->bStopThread)
	{
		int fd = static_cast<int>(reinterpret_cast<intptr_t>(Owner->SerialHandle));
		if (fd < 0)
		{
			break;
		}

		ssize_t bytesRead = read(fd, ReadBuffer, sizeof(ReadBuffer) - 1);

		if (bytesRead > 0)
		{
			ReadBuffer[bytesRead] = '\0';

			// Convert from UTF-8 to FString
			FUTF8ToTCHAR Converter(ReadBuffer, bytesRead);
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
