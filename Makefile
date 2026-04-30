CC = gcc
CFLAGS = -Wall -O3 -std=c11

#OS DETECTION
ifeq ($(OS),Windows_NT)
    # On windows we link Cryptography Next Generation Library
    LDFLAGS = -lbcrypt
    EXE = .exe
    RM = del /Q /F
    FIXPATH = $(subst /,\,$1)
else
    #On Linux and MAC we compile with pthread
    LDFLAGS = -pthread
    EXE =
    RM = rm -f
    FIXPATH = $1
endif


# Common Source Files
COMMON_SRCS = BigNumber.c Platform.c

# Source Files for test driver of the main library
DRIVER_SRCS = driver.c $(COMMON_SRCS)
DRIVER_OBJS = $(DRIVER_SRCS:.c=.o)

# Source Files for test driver of the Criptographic library
DRIVERCRIP_SRCS = driverCrip.c BigNumberCriptography.c $(COMMON_SRCS)
DRIVERCRIP_OBJS = $(DRIVERCRIP_SRCS:.c=.o)

# EXECUTABLES
TARGET_DRIVER = testdriver$(EXE)
TARGET_DRIVERCRIP = testdriverCrip$(EXE)


all: $(TARGET_DRIVER) $(TARGET_DRIVERCRIP)

driver: $(TARGET_DRIVER)

$(TARGET_DRIVER): $(DRIVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "[+] Compilare completa pentru: $@"

driverCrip: $(TARGET_DRIVERCRIP)

$(TARGET_DRIVERCRIP): $(DRIVERCRIP_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "[+] Compiled Succesfully: $@"

%.o: %.c Config.h BigNumber.h Platform.h
	$(CC) $(CFLAGS) -c $< -o $@

# Cleanup
clean:
	$(RM) $(call FIXPATH, *.o)
	$(RM) $(call FIXPATH, $(TARGET_DRIVER))
	$(RM) $(call FIXPATH, $(TARGET_DRIVERCRIP))
	@echo "[+] Cleanup Completed

.PHONY: all clean driver driverCrip