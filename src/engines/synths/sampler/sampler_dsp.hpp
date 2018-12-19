#pragma once

#include <math.h>
#include <algorithm>

#include <faust/gui/UI.h>
#include <faust/gui/meta.h>
#include <faust/dsp/dsp.h>

#include <OP1Aif.hpp>

using std::max;
using std::min;

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

#include <algorithm>
#include <cmath>
#include <math.h>

template<class T>
class PlayHead {
private:
    // SampleBufferNonAllocated<T> nonAllocatedBuffer_;
    // SampleBufferAllocated<T> allocatedBuffer_;

    typename OP1Aif<T>::AudioBuffer buffer_;
    uint32_t currentPos_;
    uint32_t startPos_;
    uint32_t endPos_;
    uint32_t length_;


    void insert(uint32_t count, T* outputBuffer, T* inputBuffer) {
        for (int i = 0; i < count; i++) {
            //printf(" %f ", inputBuffer[i]);
            outputBuffer[i] = inputBuffer[i];
        }
    }

    void pad(uint32_t count, T* outputBuffer, T value) {
        for (int i = 0; i < count; i++) {
            outputBuffer[i] = value;
        }
    }
public:
    PlayHead(
        typename OP1Aif<T>::AudioBuffer buffer,
        uint32_t startPos,
        uint32_t endPos,
        uint32_t length) :
    //    allocatedBuffer_{frameSize},
        buffer_{buffer},
        currentPos_{startPos}, // start at beginning of drum sample
        startPos_{startPos},
        endPos_{endPos},
        length_{length} {
    }

    /**
     * reset current position to start
     */
    void gate() {
        currentPos_ = startPos_;
    }

    void compute(int nFrames, T * outputBuffer) {
        if (currentPos_ > endPos_)  {
            pad(nFrames, outputBuffer, 0.0);
        }
        else if (currentPos_ + nFrames > endPos_) {
            uint32_t rem = endPos_ - currentPos_;
            insert(
                rem,
                outputBuffer,
                buffer_.data() + currentPos_);

            pad(nFrames - rem, outputBuffer + rem, 0.0);
            currentPos_ = endPos_+1;
        }
        else {
            insert(nFrames, outputBuffer, buffer_.data() + currentPos_);
            currentPos_ += nFrames;
        }
    }
};

#ifndef FAUSTCLASS
#define FAUSTCLASS sampler_dsp
#endif

class sampler_dsp : public dsp {
 private:
     int fSamplingFreq;

     // voices
     FAUSTFLOAT freqVoice1;
     FAUSTFLOAT triggerVoice1;
     FAUSTFLOAT velocityVoice1;

     // handle to loaded OP-1 drum sample
     OP1Aif<FAUSTFLOAT>* op1aif_;

     const static int NUM_DRUMS = 24;

     typedef PlayHead<FAUSTFLOAT> PlayHead_t;

     std::array<PlayHead_t *, NUM_DRUMS> playHeads_;

     static int freqToIndex(float freq) {
         // we find the cents from C3, as we currenlty assume a 2 octave midi
         // keyboard or the grid sequencer, set to a octave 0. It will not break
         // if another mapping is used, but it might behave unexpectly.
         return static_cast<uint32_t>(abs(round(12*log2(freq/130.81f)))) % 24;
     }

 public:
     sampler_dsp() :
        op1aif_{nullptr},
        playHeads_{{nullptr}} {
     }

    void setKit(OP1Aif<float>* op1aif) {
        op1aif_ = op1aif;

        printf("settKit\n");

        // delete any previous playheads
        for (auto& ph: playHeads_) {
            if (ph) {
                delete ph;
                ph = nullptr;
            }
        }

        for (int i = 0; i < playHeads_.size(); i++) {
            playHeads_[i] = new PlayHead_t{
                  op1aif_->samples(),
                  op1aif_->start(i),
                  op1aif_->end(i),
                  op1aif_->length(i)};
        }
    }

	void metadata(Meta* m) {
		m->declare("filename", "sampler");
		m->declare("name", "sampler");
	}

	virtual int getNumInputs() {
		return 0;

	}

    virtual int getNumOutputs() {
		return 1;

	}

	virtual int getInputRate(int channel) {
		int rate;
		switch (channel) {
			default: {
				rate = -1;
				break;
			}

		}
		return rate;

	}
	virtual int getOutputRate(int channel) {
		int rate;
		switch (channel) {
			case 0: {
				rate = 1;
				break;
			}
			default: {
				rate = -1;
				break;
			}

		}
		return rate;

	}

    static void classInit(int samplingFreq) {

    }

    virtual void instanceConstants(int samplingFreq) {
        fSamplingFreq = samplingFreq;
    }

    virtual void instanceResetUserInterface() {
        // voices
        freqVoice1     = FAUSTFLOAT(440.0f);
        triggerVoice1  = FAUSTFLOAT(0.0f);
        velocityVoice1 = FAUSTFLOAT(0.5f);
    }

    virtual void instanceClear() {
        // for (auto ph: playHeads_) {
        //     // reset playheads to start of sample
        //     ph->gate();
        // }
	}

    virtual void init(int samplingFreq) {
		classInit(samplingFreq);
		instanceInit(samplingFreq);
	}
	virtual void instanceInit(int samplingFreq) {
		instanceConstants(samplingFreq);
		instanceResetUserInterface();
		instanceClear();
	}

	virtual sampler_dsp* clone() {
		return new sampler_dsp();
	}

	virtual int getSampleRate() {
		return fSamplingFreq;

	}

    virtual void buildUserInterface(UI* ui_interface) {
        ui_interface->openVerticalBox("sampler");

            // handle voices start
            ui_interface->openHorizontalBox("voices");
                ui_interface->openVerticalBox("0");
                    ui_interface->openHorizontalBox("midi");
                        ui_interface->addHorizontalSlider(
                            "freq",
                            &freqVoice1,
                            440.0f, 20.0f, 1000.0f, 1.0f);
                        ui_interface->addButton("trigger", &triggerVoice1);
                        ui_interface->addHorizontalSlider(
                            "velocity",
                            &velocityVoice1,
                            0.5f, 0.0f, 1.0f, 0.00787401572f);
                    ui_interface->closeBox();
                ui_interface->closeBox();
            ui_interface->closeBox(); //
            // handle voices end

        ui_interface->closeBox(); // sampler
	}

    virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
        FAUSTFLOAT* output0 = outputs[0];
        uint32_t index = freqToIndex(freqVoice1);
        if (playHeads_[index]) {
            if (!triggerVoice1) {
                // reset playhead to start of sample
                playHeads_[index]->gate();
                for (int i = 0; i < count; i++) {
                    output0[i] = 0.0f;
                }
            }
            else {
                playHeads_[index]->compute(count, output0);
            }
        }
	}
};
