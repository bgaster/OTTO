#pragma once

#include <cstdlib>
#include <atomic>
#include <vector>
#include <array>
#include <thread>
#include <mutex>
#include <condition_variable>

/**
 * A Wrapper for ringbuffers, used for the tapemodule.
 */
class TapeBuffer {
protected:

  const static uint nTracks = 4;

  /** The current position on the tape, counted in frames from the beginning*/
  std::atomic_uint playPoint;

  /** One frame of nTracks samples */
  struct AudioFrame {
    float data[nTracks];

    float& operator[](uint i) {
      return data[i];
    }
  };

  struct Section {
    uint inIdx;
    uint outIdx;

    operator bool() {
      return inIdx != outIdx;
    }
  };

  struct RingBuffer {
    const static uint SIZE = 262144; // 2^18
    std::array<AudioFrame, SIZE> data;
    Section notWritten;
    int lengthFW;
    int lengthBW;
    uint playIdx;
    uint posAt0;

    AudioFrame& operator[](int i) {
      return data[wrapIdx(i)];
    }

    uint capacityFW();
    uint capacityBW();

    uint wrapIdx(int index) {
      return ((index %= SIZE) < 0) ? SIZE + index : index;
    }

  } buffer;

  std::thread diskThread;
  std::mutex threadLock;
  std::condition_variable readData;

  void threadRoutine();

  void movePlaypointRel(int time);

  void movePlaypointAbs(uint pos);

public:

  TapeBuffer();

  /**
   * Reads forwards along the tape, moving the playPoint.
   * @param nframes number of frames to read.
   * @param track track to read from
   * @return a vector of length nframes with the data.
   */
  std::vector<float> readFW(uint nframes, uint track);

  /**
   * Reads backwards along the tape, moving the playPoint.
   * @param nframes number of frames to read.
   * @param track track to read from
   * @return a vector of length nframes with the data. The data will be in the
   *        read order, meaning reverse.
   */
  std::vector<float> readBW(uint nframes, uint track);

  /**
   * Write data to the tape.
   * NB: The data will be written at playPoint - data.size(), meaning the end of
   * the data will be written at the current playPoint.
   * @param data the data to write. data.back() will be at playPoint - 1
   * @param track the track to write to
   * @return the amount of unwritten frames
   */
  uint writeFW(std::vector<float> data, uint track);

  /**
   * Write data to the tape.
   * NB: The data will be written beginning at playPoint
   * @param data the data to write, in reverse order. data.back() will be at
   *          playPoint, data.front() will be at playPoint + data.size();
   * @param track the track to write to
   * @return the amount of unwritten frames
   */
  uint writeBW(std::vector<float> data, uint track);

  /**
   * Jumps to another position in the tape
   * @param tapePos position to jump to
   */
  void goTo(uint tapePos);

  uint position() {
    return playPoint;
  }
};