#include <alice/library/python/decoder/stream_decoder.hpp>

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <stdexcept>

using namespace std;

int decodeFile(const char* input_filename, const char* output_filename) {
    StreamDecoder sd;
    static const size_t buff_size = 1024;
    char buff[buff_size];
    {
        // read encoded audio
        fstream fin(input_filename, ios_base::in | std::ios::binary);
        if (fin.fail()) {
            cerr << "fail open file: <" << input_filename << ">" << endl;
            return 1;
        }
        fstream fout(output_filename, ios_base::out | std::ios::binary);
        if (fout.fail()) {
            cerr << "fail open file: <" << output_filename << ">" << endl;
            return 1;
        }
        string error;
        do {
            fin.read(buff, buff_size);
            if (!fin.gcount() && fin.bad()) {
                cerr << "fail read" << endl;
                return 1;
            }
            sd.write(buff, fin.gcount());
            if (fin.eof()) {
                sd.write("", 0); //EOF marker
            }
            if (sd.getError(&error)) {
                cerr << "write to decoder failed: " << error << endl;
                return 1;
            }
            while (size_t res = sd.read(buff, buff_size)) {
                // write raw (PCM) audio
                fout.write(buff, res);
                if (fout.bad()) {
                    cerr << "fail write" << endl;
                    return 1;
                }
            }
            if (sd.getError(&error)) {
                cerr << "read from decoder failed: " << error << endl;
                return 1;
            }
        } while (!sd.eof());
    }
    return 0;
}


long parseSize(char* line) {
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    char* p = line;
    // skip WS
    while (isspace(*p)) {
        p++;
    }
    const char *pNum = p;
    while (isdigit(*p)) {
        p++;
    }
    if (p == pNum) {
        throw std::logic_error("not found expected digits");
    }

    *p = '\0';
    long n = atol(pNum);
    if (line + i != p) {
        p++;
        while (isspace(*p)) {
            p++;
        }
        if (strncmp(p, "kB", 2) == 0 or strncmp(p, "KB", 2) == 0) {
            n *= 1024;
        } else if (strncmp(p, "kB", 2) == 0 or strncmp(p, "KB", 2) == 0) {
            n *= 1024 * 1024;
        }
    }
    return n;
}

//return used resident memory (bytes)
long getVmRss() {
    fstream fin("/proc/self/status", ios_base::in);
    int result = -1;
    char line[128];

    while (not fin.getline(line, 128).eof()) {
        if (not fin.good()) {
            throw std::logic_error("fail on read /proc/self/status");
        }

        if (strncmp(line, "VmRSS:", 6) == 0) {
            result = parseSize(line + 6);
            break;
        }
    }
    return result;
}


int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " input-filename output-filename [NUM_RUNS]" << endl;
        cerr << " if NUM_RUNS > 50000 check leaking size (expected 0 bytes per RUN)" << endl;
        return 1;
    }

    StreamDecoder::initGlobalEnviroment(40 /*==AV_LOG_VERBOSE*/);

    if (!StreamDecoder::test()) {
        cerr << "Fail StreamDecoder::DataQueue::Test()" << endl;
        return 1;
    }

    const char* input_filename = argv[1];
    const char* output_filename = argv[2];
    int num_runs = 1;
    if (argv[3]) {
        num_runs = atoi(argv[3]);
    }
    int warmRuns = 10;
    long rssAfterFirstRun = 0;
    for (int i = 0; i < num_runs; ++i) {
        int result = decodeFile(input_filename, output_filename);
        if (result != 0) {
            return result;
        }
        if ((i % 100) == 0) {
            cout << "#" << i << " VmRSS=" << getVmRss() << endl;
        }
        if (i == warmRuns) {
            rssAfterFirstRun = getVmRss();
        }
    }
    if (num_runs >= 50000) {
        long lastRss = getVmRss();
        long leak = lastRss - rssAfterFirstRun;
        long leakPerRun = leak / (num_runs - warmRuns);
        if (leakPerRun != 0) {
            cerr << "has not zero leak_per_run=" << leakPerRun
                << " rss_after_first_run=" << rssAfterFirstRun
                << " rss_after_" << num_runs << "_runs=" << lastRss
                << endl;
            return 1;
        }
    }

    cout << "test StreamDecoder is ok!" << endl;
    return 0;
}
