#include <memory>
#include <iostream>
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>

#include "RtMidi.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_split.h"
#include "absl/strings/numbers.h"
#include "absl/log/log.h"

#include "audio.h"
#include "synth.h"

ABSL_FLAG(bool, list, false, "List midi ports");
ABSL_FLAG(int, port, -1, "Midi port to use");
ABSL_FLAG(double, C0, 8.175, "Frequency of C0");
ABSL_FLAG(std::string, cents, "", "Interval offsets in cents");

void compute_frequency_table(double C0, const std::vector<double>& cents) {
    extern int16_t frequency[];
    const double TRT = std::pow(2.0, 1.0/12.0);
    double octave = cents[12];
    for(size_t m=12; m<128; m++) {
        size_t n = m - 12;
        double c = octave * (n/12);
        if (n%12) c += cents[n%12];
        frequency[m] = C0 * std::pow(TRT, c/100.0);
    }
}


void tty_raw(int fd) {
    struct termios t;
    tcgetattr(fd, &t);
    t.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(fd, TCSAFLUSH, &t);
}

void tty_restore(int fd) {
    struct termios t;
    tcgetattr(fd, &t);
    t.c_lflag |= (ECHO | ICANON);
    tcsetattr(fd, TCSAFLUSH, &t);
}

void set_max_priority(void) {
    pthread_t id = pthread_self();
    pthread_attr_t attr;
    int policy = 0, max_prio = 0;

    pthread_attr_init(&attr);
    pthread_attr_getschedpolicy(&attr, &policy);
    printf("policy = %d\n", policy);
    max_prio = sched_get_priority_max(policy);
    printf("max priority = %d\n", max_prio);
    pthread_setschedprio(id, max_prio);
    pthread_attr_destroy(&attr);
}

void set_realtime(void) {
    struct sched_param p = {90};
    if (sched_setscheduler(0, SCHED_FIFO, &p) == -1) {
        perror("sched_setscheduler");
    }
}

void midi_listener(double dt, std::vector<uint8_t>* message, void *userdata) {
    synth_midi(message->data());
}

std::string_view midi_api(int api) {
    switch(api) {
        case RtMidi::MACOSX_CORE: return "OS-X CoreMidi";
        case RtMidi::WINDOWS_MM: return "Windows MultiMedia";
        case RtMidi::UNIX_JACK: return "Jack Client";
        case RtMidi::LINUX_ALSA: return "Linux ALSA";
        case RtMidi::RTMIDI_DUMMY: return "RtMidi Dummy";
        default:
            return "Unknown";
    }
}

int main(int argc, char *argv[]) {
    absl::ParseCommandLine(argc, argv);

    auto midi = std::make_unique<RtMidiIn>();
    int nports = midi->getPortCount();
    if (absl::GetFlag(FLAGS_list)) {
        std::cout << "RtMidi Input API: " << midi_api(midi->getCurrentApi()) << "\n";
        for(int i=0; i<nports; ++i) {
            auto name = midi->getPortName(i);
            std::cout << "  Input port " << i << ": " << name << "\n";
        }
        return 0;
    }

    const auto& cents = absl::GetFlag(FLAGS_cents);
    if (!cents.empty()) {
        std::vector<double> values = {0.0};
        for(const auto& val : absl::StrSplit(cents, ',')) {
            double v;
            if (absl::SimpleAtod(val, &v)) {
                values.push_back(v);
            } else {
                LOG(FATAL) << "Could not parse " << val << " as a number";
            }
        }
        compute_frequency_table(absl::GetFlag(FLAGS_C0), values);
    }

    int port = absl::GetFlag(FLAGS_port);
    if (port < 0) {
        midi->openVirtualPort();
    } else {
        midi->openPort(port);
    }
    midi->setCallback(midi_listener, nullptr);
    midi->ignoreTypes(false, false, false);

    set_realtime();
    set_max_priority();

    audio_state_t audio;
    audio_init(&audio);
    synth_init();
    tty_raw(0);
    fcntl(0, F_SETFL, O_NONBLOCK);
    printf("Starting minisynth.  Press 'q' to quit.\n");
    int16_t buf[AUDIO_BUFFER_SZ];
    uint64_t tstep = 0;
    for(;;) {
        if (tstep % AUDIO_SAMPLE_RATE == 0) {
            uint8_t ch = 0;
            int n = read(0, &ch, 1);
            if (n == 1 && (ch == 'q' || ch=='Q')) {
                break;
            }
        }
        for(uint32_t i=0; i<AUDIO_BUFFER_SZ; ++i, ++tstep) {
            buf[i] = synth_value(tstep);
        }
        audio_send(&audio, buf, AUDIO_BUFFER_SZ);
    }
    tty_restore(0);
    return 0;
}
