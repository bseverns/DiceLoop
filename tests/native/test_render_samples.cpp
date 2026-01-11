#include <unity.h>

#include "Arduino.h"
#include "audio_pipeline.h"
#include "chaos.h"
#include "controls.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct WavData {
    int sample_rate = 0;
    int channels = 0;
    std::vector<int16_t> samples;
};

uint32_t read_u32(const uint8_t *data) {
    return static_cast<uint32_t>(data[0]) |
           (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) |
           (static_cast<uint32_t>(data[3]) << 24);
}

uint16_t read_u16(const uint8_t *data) {
    return static_cast<uint16_t>(data[0]) |
           (static_cast<uint16_t>(data[1]) << 8);
}

bool load_wav(const fs::path &path, WavData *out) {
    if (!out) {
        return false;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
    if (bytes.size() < 44) {
        return false;
    }
    if (std::string(reinterpret_cast<char *>(bytes.data()), 4) != "RIFF") {
        return false;
    }
    if (std::string(reinterpret_cast<char *>(bytes.data() + 8), 4) != "WAVE") {
        return false;
    }

    uint16_t audio_format = 0;
    uint16_t channels = 0;
    uint32_t sample_rate = 0;
    uint16_t bits_per_sample = 0;
    size_t data_offset = 0;
    size_t data_size = 0;

    size_t offset = 12;
    while (offset + 8 <= bytes.size()) {
        std::string chunk_id(reinterpret_cast<char *>(bytes.data() + offset), 4);
        uint32_t chunk_size = read_u32(bytes.data() + offset + 4);
        size_t chunk_start = offset + 8;
        if (chunk_start + chunk_size > bytes.size()) {
            return false;
        }
        if (chunk_id == "fmt ") {
            if (chunk_size < 16) {
                return false;
            }
            audio_format = read_u16(bytes.data() + chunk_start);
            channels = read_u16(bytes.data() + chunk_start + 2);
            sample_rate = read_u32(bytes.data() + chunk_start + 4);
            bits_per_sample = read_u16(bytes.data() + chunk_start + 14);
        } else if (chunk_id == "data") {
            data_offset = chunk_start;
            data_size = chunk_size;
        }
        offset = chunk_start + chunk_size;
        if (chunk_size % 2 == 1) {
            offset += 1;
        }
    }

    if (audio_format != 1 || bits_per_sample != 16 || channels == 0 ||
        data_offset == 0 || data_size == 0) {
        return false;
    }

    if (data_offset + data_size > bytes.size()) {
        return false;
    }
    if (data_size % 2 != 0) {
        return false;
    }

    size_t sample_count = data_size / 2;
    out->samples.resize(sample_count);
    const uint8_t *data = bytes.data() + data_offset;
    for (size_t i = 0; i < sample_count; ++i) {
        out->samples[i] = static_cast<int16_t>(read_u16(data + i * 2));
    }
    out->sample_rate = static_cast<int>(sample_rate);
    out->channels = static_cast<int>(channels);
    return true;
}

bool write_wav(const fs::path &path, const WavData &data) {
    if (data.sample_rate <= 0 || data.channels <= 0) {
        return false;
    }

    uint32_t data_bytes = static_cast<uint32_t>(data.samples.size() * sizeof(int16_t));
    uint32_t riff_size = 36 + data_bytes;
    uint16_t channels = static_cast<uint16_t>(data.channels);
    uint32_t sample_rate = static_cast<uint32_t>(data.sample_rate);
    uint32_t byte_rate =
        static_cast<uint32_t>(sample_rate * channels * sizeof(int16_t));
    uint16_t block_align = static_cast<uint16_t>(channels * sizeof(int16_t));
    uint16_t bits_per_sample = 16;

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.write("RIFF", 4);
    file.write(reinterpret_cast<const char *>(&riff_size), 4);
    file.write("WAVE", 4);
    file.write("fmt ", 4);

    uint32_t fmt_size = 16;
    uint16_t audio_format = 1;
    file.write(reinterpret_cast<const char *>(&fmt_size), 4);
    file.write(reinterpret_cast<const char *>(&audio_format), 2);
    file.write(reinterpret_cast<const char *>(&channels), 2);
    file.write(reinterpret_cast<const char *>(&sample_rate), 4);
    file.write(reinterpret_cast<const char *>(&byte_rate), 4);
    file.write(reinterpret_cast<const char *>(&block_align), 2);
    file.write(reinterpret_cast<const char *>(&bits_per_sample), 2);

    file.write("data", 4);
    file.write(reinterpret_cast<const char *>(&data_bytes), 4);
    file.write(reinterpret_cast<const char *>(data.samples.data()), data_bytes);
    return true;
}

fs::path find_sample_path() {
    const fs::path relative = fs::path("docs") / "sample" / "sample.wav";
    const fs::path cwd = fs::current_path();
    if (fs::exists(cwd / relative)) {
        return fs::weakly_canonical(cwd / relative);
    }
    fs::path cursor = fs::path(__FILE__).parent_path();
    for (int depth = 0; depth < 6; ++depth) {
        if (fs::exists(cursor / relative)) {
            return fs::weakly_canonical(cursor / relative);
        }
        if (cursor.has_parent_path()) {
            cursor = cursor.parent_path();
        } else {
            break;
        }
    }
    return {};
}

struct RenderConfig {
    const char *id;
    const char *stack_id;
    float density_norm;
    float noise_norm;
};

std::vector<int16_t> render_samples(const WavData &input, const RenderConfig &cfg) {
    std::vector<int16_t> output = input.samples;

    DirtStackInfo stack{};
    if (!curatedDirtStackById(cfg.stack_id, &stack)) {
        return output;
    }

    setActiveDirtStages(stack.mask);
    setStutterTimingMode(StutterTimingMode::Probability);
    setChaosModulatorsEnabled(false);
    resetDirtStateForTest();
    randomSeed(1337);

    density = static_cast<int>(cfg.density_norm * 100.0f);
    noiseAmount = static_cast<int>(cfg.noise_norm * 60.0f);
    setDirtControlSnapshot(cfg.density_norm, cfg.noise_norm, 1.0f);

    for (size_t i = 0; i < output.size(); ++i) {
        float sample = static_cast<float>(input.samples[i]) / 32768.0f;
        float processed = processDirt(sample);
        processed = constrain(processed, -1.0f, 1.0f);
        output[i] = static_cast<int16_t>(processed * 32767.0f);
    }

    return output;
}

}  // namespace

void test_render_samples() {
    dice_loop_stub::reset_state();
    setupChaos();

    fs::path sample_path = find_sample_path();
    TEST_ASSERT_MESSAGE(!sample_path.empty(), "Sample WAV not found.");

    WavData input{};
    TEST_ASSERT_TRUE_MESSAGE(load_wav(sample_path, &input),
                             "Failed to load sample WAV.");

    fs::path output_dir = sample_path.parent_path() / "outputs";
    fs::create_directories(output_dir);

    const RenderConfig configs[] = {
        {"full_send_dense", "full_send", 0.85f, 0.75f},
        {"stutter_gate_sparse", "stutter_gate", 0.55f, 0.20f},
        {"fuzz_bloom_warm", "fuzz_bloom", 0.35f, 0.65f},
        {"crush_hiccups_mid", "crush_hiccups", 0.45f, 0.40f},
    };

    for (const auto &cfg : configs) {
        WavData output = input;
        output.samples = render_samples(input, cfg);
        std::string filename = "sample_" + std::string(cfg.id) + ".wav";
        fs::path output_path = output_dir / filename;
        TEST_ASSERT_TRUE_MESSAGE(write_wav(output_path, output),
                                 "Failed to write rendered WAV.");
        TEST_ASSERT_TRUE_MESSAGE(fs::exists(output_path),
                                 "Rendered WAV missing after write.");
    }
}
