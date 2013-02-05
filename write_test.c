#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

int64_t TimeDiffMicros(const struct timespec* start, const struct timespec* end) {
  int64_t sec_diff = end->tv_sec - start->tv_sec;
  assert(sec_diff >= 0);
  return 1000000 * sec_diff + (end->tv_nsec - start->tv_nsec) / 1000;
}

void FillBuffer(char* buf, size_t len) {
  for (size_t i = 0; i < len; i++) {
    buf[i] = (char)rand();
    if (!buf[i]) {
      buf[i] = 'g';
    }
  }
}

void WriteChunks(int fd, size_t chunk_size, int no_chunks, char* chunk_buf) {
}

const size_t kMaxChunkSizeBytes = 1024 * 1024;  // 1MB
const size_t kTotalSize = 200 * 1024 * 1024;
const char* kFileName = "/data/local/tmp/test_data";

int main() {
  char* buf;
  struct timespec start, end;
  int fd;
  int ret = 0;

  srand(0);
  buf = (char*) malloc(kMaxChunkSizeBytes);
  if (!buf) {
    perror("malloc");
    return 1;
  }
  FillBuffer(buf, kMaxChunkSizeBytes);

  const size_t sizes[] = {/*8, 16, 32, 64, 128, 512, 1024,*/ 4096 /*, 8192, */
                          /* kMaxChunkSizeBytes */};
  for(int i = 0; i < sizeof(sizes)/sizeof(size_t); i++) {
    size_t chunk_size = sizes[i];
    int64_t time_millis;
    const size_t kBufLen = 30;
    char filename[kBufLen];
    snprintf(filename, kBufLen, "%s%d", kFileName, i);

    fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0664);
    if (fd == -1) {
      perror("open");
      ret = 1;
      goto finalize;
    }
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int j = 0; j < kTotalSize / chunk_size; j++) {
      ssize_t bytes_written;
      do {
        errno = 0;
        bytes_written = write(fd, buf, chunk_size);
      } while (bytes_written == -1 && errno == EINTR);
      if (errno) {
        perror("write");
        ret = 1;
        goto finalize;
      }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    time_millis = TimeDiffMicros(&start, &end) / 1000;
    printf("writing by chunks of %" PRId32 " bytes took %" PRId64 " ms,"
           " speed= %" PRId64 " KB/S\n",
           chunk_size, time_millis, kTotalSize/time_millis);
    close(fd);
    unlink(kFileName);
  }
finalize:
  free(buf);
  return ret;
}
