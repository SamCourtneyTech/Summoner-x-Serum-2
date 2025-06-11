#include "ParameterNormalizer.h"
#include <vector>
#include <regex>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
const std::vector<float> frequencies = {  };
float getRandomFValue() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}
std::pair<std::string, float> normalizeValue(const std::string& name, const std::string& value) {
    float normalizedValue = getRandomFValue();
    
    // Map Serum 1 parameter names to Serum 2 parameter names
    std::string serum2Name = name;
    
    // Envelope mappings
    if (name == "Env1 Atk") serum2Name = "Env 1 Attack";
    else if (name == "Env1 Hold") serum2Name = "Env 1 Hold";
    else if (name == "Env1 Dec") serum2Name = "Env 1 Decay";
    else if (name == "Env1 Sus") serum2Name = "Env 1 Sustain";
    else if (name == "Env1 Rel") serum2Name = "Env 1 Release";
    
    // Oscillator mappings
    else if (name == "A Vol") serum2Name = "A Level";
    else if (name == "B Vol") serum2Name = "B Level";
    else if (name == "A WTPos") serum2Name = "A WT Pos";
    else if (name == "B WTPos") serum2Name = "B WT Pos";
    else if (name == "Sub Osc Level") serum2Name = "Sub Level";
    else if (name == "SubOscShape") serum2Name = "Sub Shape";
    else if (name == "SubOscOctave") serum2Name = "Sub Octave";
    
    // Unison mappings
    else if (name == "A UniBlend") serum2Name = "A Uni Blend";
    else if (name == "B UniBlend") serum2Name = "B Uni Blend";
    else if (name == "A UniDet") serum2Name = "A Uni Detune";
    else if (name == "B UniDet") serum2Name = "B Uni Detune";
    
    // Filter mappings
    else if (name == "Fil Cutoff") serum2Name = "Filter 1 Freq";
    else if (name == "Fil Type") serum2Name = "Filter 1 Type";
    else if (name == "FX Fil Type") serum2Name = "Filter 2 Type";
    else if (name == "FX Fil Freq") serum2Name = "Filter 2 Freq";
    
    // FX/Effects mappings (underscore to space, proper capitalization)
    else if (name == "Comp_Wet") serum2Name = "Comp Wet";
    else if (name == "Cho_Dep") serum2Name = "Cho Dep";
    else if (name == "Cho_Feed") serum2Name = "Cho Feed";
    else if (name == "Cho_Rate") serum2Name = "Cho Rate";
    else if (name == "Cho_Dly") serum2Name = "Cho Dly";
    else if (name == "Cho_Dly2") serum2Name = "Cho Dly2";
    else if (name == "Cho_Filt") serum2Name = "Cho Filt";
    else if (name == "Phs_Rate") serum2Name = "Phs Rate";
    else if (name == "Phs_Stereo") serum2Name = "Phs Stereo";
    else if (name == "Phs_Frq") serum2Name = "Phs Frq";
    else if (name == "Flg_Rate") serum2Name = "Flg Rate";
    else if (name == "Flg_Stereo") serum2Name = "Flg Stereo";
    else if (name == "Dist_Mode") serum2Name = "Dist Mode";
    else if (name == "Dist_PrePost") serum2Name = "Dist PrePost";
    else if (name == "Dist_BW") serum2Name = "Dist BW";
    else if (name == "Dist_Freq") serum2Name = "Dist Freq";
    else if (name == "Dly_Link") serum2Name = "Dly Link";
    else if (name == "Dly_TimL") serum2Name = "Dly TimL";
    else if (name == "Dly_TimR") serum2Name = "Dly TimR";
    else if (name == "Dly_BW") serum2Name = "Dly BW";
    else if (name == "Dly_Mode") serum2Name = "Dly Mode";
    else if (name == "Dly_Freq") serum2Name = "Dly Freq";
    else if (name == "Cmp_Thr") serum2Name = "Cmp Thr";
    else if (name == "Cmp_Att") serum2Name = "Cmp Att";
    else if (name == "Cmp_Rel") serum2Name = "Cmp Rel";
    else if (name == "CmpGain") serum2Name = "Cmp Gain";
    else if (name == "CmpMBnd") serum2Name = "Cmp MBnd";
    else if (name == "EQ_FrqL") serum2Name = "EQ FrqL";
    else if (name == "EQ_FrqH") serum2Name = "EQ FrqH";
    else if (name == "EQ_VolL") serum2Name = "EQ VolL";
    else if (name == "EQ_VolH") serum2Name = "EQ VolH";
    else if (name == "EQ_TypL") serum2Name = "EQ TypL";
    else if (name == "EQ_TypH") serum2Name = "EQ TypH";
    else if (name == "Hyp_Retrig") serum2Name = "Hyp Retrig";
    else if (name == "Hyp_Unison") serum2Name = "Hyp Unison";
    
    try {
        // Use the mapped Serum 2 name for comparisons
        if (serum2Name == "Env 1 Attack" || serum2Name == "Env 1 Hold" || serum2Name == "Env 1 Decay" || serum2Name == "Env 1 Release")
            normalizedValue = normalizeMsS(serum2Name, value);
        else if (serum2Name == "Env 1 Sustain")
            normalizedValue = normalizeDbToF(serum2Name, value);
        else if (serum2Name == "A Pan" || serum2Name == "B Pan")
            normalizedValue = normalizePanToF(serum2Name, value);
        else if (serum2Name == "Cho Dep")
            normalizedValue = choDepthToPercentage(serum2Name, value);
        else if (serum2Name == "B WT Pos")
            normalizedValue = wtToMidi(serum2Name, value);// will need to figure out alternative to this- have chat select custom wavetable???
        else if (serum2Name == "A WT Pos")
            normalizedValue = wtToMidi(serum2Name, value);
        else if (serum2Name == "A Level")
            normalizedValue = percentageToMacro(serum2Name, value);
        else if (name == "Noise Level" || serum2Name == "B Level" || serum2Name == "Sub Level")
            normalizedValue = percentageToMacro(serum2Name, value);
        else if (serum2Name == "Cho Feed")
            normalizedValue = choFeedToMacro(serum2Name, value);
        else if (value.find('%') != std::string::npos && value.find('(') == std::string::npos)
            normalizedValue = percentageToMacro(name, value);
        else if (serum2Name == "B Unison")
            normalizedValue = unisonToMacro(serum2Name, value);
        else if (serum2Name == "A Unison")
            normalizedValue = unisonToMacro(serum2Name, value);
        else if (serum2Name == "A Octave")
            normalizedValue = octToMidi(serum2Name, value);
        else if (serum2Name == "A Semi" || serum2Name == "B Semi")
            normalizedValue = semiToMacro(serum2Name, value);
        else if (serum2Name == "A Fine" || serum2Name == "B Fine")
            normalizedValue = fineToMacro(serum2Name, value);
        else if (serum2Name == "Filter 1 Freq")
            normalizedValue = frequencyToPercentage(serum2Name, value);
        else if (serum2Name == "Filter 1 Type" || serum2Name == "Filter 2 Type")
            normalizedValue = filterTypeToMacro(serum2Name, value);
        else if (serum2Name == "Dist Mode")
            normalizedValue = distortionTypeToMacro(serum2Name, value);
        else if (serum2Name == "A Uni Blend" || serum2Name == "B Uni Blend" || serum2Name == "Comp Wet")
            normalizedValue = uniblendToF(serum2Name, value);
        else if (serum2Name == "Sub Shape")
            normalizedValue = subShapeToMacro(serum2Name, value);
        else if (serum2Name == "B Octave" || serum2Name == "Sub Octave")
            normalizedValue = octToMidi(serum2Name, value);
        else if (serum2Name == "Hyp Retrig")
            normalizedValue = onToPercentage(serum2Name, value);
        else if (serum2Name == "Hyp Unison")
            normalizedValue = hypUnisonToMacro(serum2Name, value);
        else if (serum2Name == "A Uni Detune" || serum2Name == "B Uni Detune")
            normalizedValue = uniDetToMacro(serum2Name, value);
        else if (serum2Name == "Dist PrePost")
            normalizedValue = distPrePostToMacro(serum2Name, value);
        else if (serum2Name == "Dist BW")
            normalizedValue = distBwToPercentage(serum2Name, value);
        else if (serum2Name == "Cho Rate" || serum2Name == "Phs Rate" || serum2Name == "Flg Rate")
            normalizedValue = phaseRateToMacro(serum2Name, value);
        else if (serum2Name == "Flg Stereo" || serum2Name == "Phs Stereo")
            normalizedValue = degreesToPercentage(serum2Name, value);
        else if (serum2Name == "Cho Dly" || serum2Name == "Cho Dly2")
            normalizedValue = choDlyToPercentage(serum2Name, value);
        else if (serum2Name == "Cho Dep")
            normalizedValue = choDepToPercentage(serum2Name, value);
        else if (serum2Name == "Cho Filt")
            normalizedValue = choFiltToPercentage(serum2Name, value);
        else if (serum2Name == "Dist Freq")
            normalizedValue = distFreqToPercentage(serum2Name, value);
        else if (serum2Name == "Dly Freq")
            normalizedValue = dlyFreqToPercentage(serum2Name, value);
        else if (serum2Name == "Phs Frq")
            normalizedValue = phsFrqToPercentage(serum2Name, value);
        else if (serum2Name == "EQ FrqL" || serum2Name == "EQ FrqH")
            normalizedValue = EQfrqToPercentage(serum2Name, value);
        else if (serum2Name == "Filter 2 Freq")
            normalizedValue = distFreqToPercentage(serum2Name, value);
        else if (serum2Name == "Dly Link")
            normalizedValue = onToPercentage(serum2Name, value);
        else if (serum2Name == "Dly TimL" || serum2Name == "Dly TimR")
            normalizedValue = delayTimeToPercentage(serum2Name, value);
        else if (serum2Name == "Dly BW")
            normalizedValue = dlyBwToPercentage(serum2Name, value);
        else if (serum2Name == "Dly Mode")
            normalizedValue = dlyModeToPercentage(serum2Name, value);
        else if (serum2Name == "Cmp Thr")
            normalizedValue = cmpThrToPercentage(serum2Name, value);
        else if (serum2Name == "Cmp Att")
            normalizedValue = cmpAttToPercentage(serum2Name, value);
        else if (serum2Name == "Cmp Rel")
            normalizedValue = cmpRelToPercentage(serum2Name, value);
        else if (serum2Name == "Cmp Gain")
            normalizedValue = cmpGainToPercentage(serum2Name, value);
        else if (serum2Name == "Cmp MBnd")
            normalizedValue = CmpMBndToPercentage(serum2Name, value);
        else if (serum2Name == "EQ VolL" || serum2Name == "EQ VolH")
            normalizedValue = eqVolToPercentage(serum2Name, value);
        else if (serum2Name == "EQ TypL" || serum2Name == "EQ TypH")
            normalizedValue = eqTypToPercentage(serum2Name, value);
        else if (value == "on" || value == "off")
            normalizedValue = onToPercentage(name, value);
        else if (name == "Decay")
            normalizedValue = decayToF(name, value);
        else
            normalizedValue = std::stof(value);
        //missing fil pan
        //missing ratio
    }
    catch (...) {
        normalizedValue = getRandomFValue();
    }
    return { serum2Name, normalizedValue };
}
const std::vector<float> serum_ms_values = {
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.1, 0.1, 0.2, 0.2, 0.4, 0.5, 0.7, 1.0, 1.4, 1.8, 2.4, 3.1, 4.0,
    5.0, 6.2, 7.7, 9.5, 12, 14, 17, 20, 24, 28, 32, 38, 44, 51, 59, 67, 77, 87, 99, 112, 127, 142, 160, 179, 199,
    222, 247, 274, 303, 334, 368, 405, 445, 487, 533, 583, 636, 692, 753, 818, 887, 961, 1040, 1120, 1210, 1310,
    1410, 1510, 1630, 1750, 1870, 2010, 2150, 2300, 2460, 2620, 2800, 2980, 3170, 3380, 3590, 3820, 4050, 4300,
    4560, 4830, 5110, 5410, 5720, 6040, 6380, 6740, 7110, 7490, 7900, 8320, 8760, 9210, 9690, 10200, 10700, 11200,
    11800, 12400, 13000, 13600, 14200, 14900, 15600, 16300, 17100, 17800, 18600, 19500, 20300, 21200, 22200, 23100,
    24100, 25100, 26200, 27300, 28400, 29600, 30800, 32000
};
float normalizeMsS(const std::string& name, const std::string& value) {
    std::regex re(R"(([+-]?[0-9]*\.?[0-9]+)\s*(ms|s))");
    std::smatch match;
    if (!std::regex_search(value, match, re))
        return getRandomFValue();
    float timeValue = std::stof(match[1].str());
    std::string unit = match[2].str();
    if (unit == "s") {
        timeValue *= 1000.0f;
    }
    auto it = std::lower_bound(serum_ms_values.begin(), serum_ms_values.end(), timeValue);
    if (it == serum_ms_values.end()) {
        return 1.0f;
    }
    int index = std::distance(serum_ms_values.begin(), it);
    return static_cast<float>(index) / static_cast<float>(serum_ms_values.size() - 1);
}
const std::vector<float> dB_values = {
    -INFINITY, -84.2, -72.1, -65.1, -60.1, -56.2, -53.0, -50.3, -48.0, -46.0, -44.2, -42.5, -41.0,
    -39.6, -38.3, -37.1, -36.0, -34.9, -33.9, -33.0, -32.1, -31.3, -30.5, -29.7, -28.9, -28.2, -27.6,
    -26.9, -26.3, -25.7, -25.1, -24.5, -23.9, -23.4, -22.9, -22.4, -21.9, -21.4, -21.0, -20.5, -20.1,
    -19.6, -19.2, -18.8, -18.4, -18.0, -17.6, -17.3, -16.9, -16.5, -16.2, -15.8, -15.5, -15.2, -14.9,
    -14.5, -14.2, -13.9, -13.6, -13.3, -13.0, -12.7, -12.5, -12.2, -11.9, -11.6, -11.4, -11.1, -10.9,
    -10.6, -10.3, -10.1, -9.9, -9.6, -9.4, -9.1, -8.9, -8.7, -8.5, -8.2, -8.0, -7.8, -7.6, -7.4, -7.2,
    -7.0, -6.8, -6.6, -6.4, -6.2, -6.0, -5.8, -5.6, -5.4, -5.2, -5.0, -4.9, -4.7, -4.5, -4.3, -4.2,
    -4.0, -3.8, -3.6, -3.5, -3.3, -3.1, -3.0, -2.8, -2.7, -2.5, -2.3, -2.2, -2.0, -1.9, -1.7, -1.6,
    -1.4, -1.3, -1.1, -1.0, -0.8, -0.7, -0.6, -0.4, -0.3, -0.1, 0.0
};
float normalizeDbToF(const std::string& name, const std::string& value) {
    std::regex re(R"(([+-]?[0-9]*\.?[0-9]+)\s*dB)");
    std::smatch match;
    float dBValue = -INFINITY;
    if (value == "-inf dB") {
        dBValue = -INFINITY;
    }
    else if (std::regex_search(value, match, re)) {
        dBValue = std::stof(match[1].str());
    }
    else {
        return getRandomFValue();
    }
    auto it = std::lower_bound(dB_values.begin(), dB_values.end(), dBValue);
    if (it == dB_values.end()) {
        return 1.0f;
    }
    int index = std::distance(dB_values.begin(), it);
    return static_cast<float>(index) / static_cast<float>(dB_values.size() - 1);
}
float normalizePanToF(const std::string& name, const std::string& value) {
    try {
        float panValue = std::stof(value);
        panValue = std::clamp(panValue, -50.0f, 50.0f);
        return (panValue + 50.0f) / 100.0f;
    }
    catch (...) {
        return getRandomFValue();
    }
}
float choDepthToPercentage(const std::string& name, const std::string& value) {
    try {
        float msValue = std::stof(value);
        const std::vector<std::pair<float, float>> msToPercentageMap = {
            {0.0f, 0.0f}, {0.1f, 5.0f}, {0.2f, 8.0f}, {0.3f, 10.0f}, {0.4f, 12.0f}, {0.5f, 14.0f},
            {0.6f, 15.0f}, {0.7f, 16.0f}, {0.8f, 17.0f}, {0.9f, 19.0f}, {1.0f, 20.0f}, {1.1f, 21.0f},
            {1.3f, 22.0f}, {1.4f, 23.0f}, {1.5f, 24.0f}, {1.6f, 25.0f}, {1.8f, 26.0f}, {1.9f, 27.0f},
            {2.0f, 28.0f}, {2.2f, 29.0f}, {2.3f, 30.0f}, {2.5f, 31.0f}, {2.7f, 32.0f}, {2.8f, 33.0f},
            {3.0f, 34.0f}, {3.2f, 35.0f}, {3.4f, 36.0f}, {3.6f, 37.0f}, {3.8f, 38.0f}, {4.0f, 39.0f},
            {4.2f, 40.0f}, {4.4f, 41.0f}, {4.6f, 42.0f}, {4.8f, 43.0f}, {5.0f, 44.0f}, {5.4f, 45.0f},
            {5.5f, 46.0f}, {5.7f, 47.0f}, {6.0f, 48.0f}, {6.2f, 49.0f}, {6.5f, 50.0f}, {6.8f, 51.0f},
            {7.0f, 52.0f}, {7.3f, 53.0f}, {7.6f, 54.0f}, {7.9f, 55.0f}, {8.2f, 56.0f}, {8.4f, 57.0f},
            {8.9f, 58.0f}, {9.2f, 59.0f}, {9.4f, 60.0f}, {9.7f, 61.0f}, {10.0f, 62.0f}, {10.3f, 63.0f},
            {10.6f, 64.0f}, {11.0f, 65.0f}, {11.3f, 66.0f}, {11.7f, 67.0f}, {12.0f, 68.0f}, {12.4f, 69.0f},
            {12.7f, 70.0f}, {13.1f, 71.0f}, {13.5f, 72.0f}, {13.9f, 73.0f}, {14.4f, 74.0f}, {14.6f, 75.0f},
            {15.0f, 76.0f}, {15.4f, 77.0f}, {15.8f, 78.0f}, {16.2f, 79.0f}, {16.6f, 80.0f}, {17.1f, 81.0f},
            {17.5f, 82.0f}, {17.9f, 83.0f}, {18.3f, 84.0f}, {18.8f, 85.0f}, {19.2f, 86.0f}, {19.7f, 87.0f},
            {20.1f, 88.0f}, {20.8f, 89.0f}, {21.1f, 90.0f}, {21.5f, 91.0f}, {22.0f, 92.0f}, {22.5f, 93.0f},
            {23.0f, 94.0f}, {23.5f, 95.0f}, {24.0f, 96.0f}, {24.5f, 97.0f}, {25.0f, 98.0f}, {25.7f, 99.0f},
            {26.0f, 100.0f}
        };
        for (const auto& pair : msToPercentageMap) {
            if (msValue == pair.first) return pair.second / 100.0f;
        }
        auto upper = std::upper_bound(msToPercentageMap.begin(), msToPercentageMap.end(), msValue,
            [](float val, const std::pair<float, float>& pair) {
                return val < pair.first;
            });
        if (upper == msToPercentageMap.begin()) return msToPercentageMap.front().second / 100.0f;
        if (upper == msToPercentageMap.end()) return msToPercentageMap.back().second / 100.0f;
        auto lower = std::prev(upper);
        float lowerMs = lower->first, lowerPercent = lower->second;
        float upperMs = upper->first, upperPercent = upper->second;
        float interpolatedPercentage = lowerPercent + ((msValue - lowerMs) / (upperMs - lowerMs)) * (upperPercent - lowerPercent);
        return std::clamp(interpolatedPercentage / 100.0f, 0.0f, 1.0f);
    }
    catch (...) {
        return getRandomFValue();
    }
}
float wtToMidi(const std::string& name, const std::string& value) {
    static const std::unordered_map<std::string, float> wtToMidiMap = {
        {"1", 14.0f / 100.0f}, {"2", 18.0f / 100.0f}, {"3", 30.0f / 100.0f},
        {"4", 45.0f / 100.0f}, {"5", 59.0f / 100.0f}, {"6", 74.0f / 100.0f},
        {"7", 100.0f / 100.0f},
        {"Sine", 14.0f / 100.0f}, {"Saw", 18.0f / 100.0f}, {"Triangle", 30.0f / 100.0f},
        {"Square", 45.0f / 100.0f}, {"Pulse", 59.0f / 100.0f}, {"Half Pulse", 74.0f / 100.0f},
        {"Inv-Phase saw", 100.0f / 100.0f}
    };
    auto it = wtToMidiMap.find(value);
    if (it != wtToMidiMap.end()) {
        return it->second;
    }
    return getRandomFValue();
}
float percentageToMacro(const std::string& name, const std::string& value) {
    try {
        float percentageValue = std::stof(value.substr(0, value.find('%')));
        return std::clamp(percentageValue / 100.0f, 0.0f, 1.0f);
    }
    catch (...) {
        return getRandomFValue();
    }
}
float choFeedToMacro(const std::string& name, const std::string& value) {
    try {
        float intensity = std::stof(value.substr(0, value.find('%')));
        if (intensity < 0.0f || intensity > 95.0f)
            return getRandomFValue();
        return std::clamp(intensity / 95.0f, 0.0f, 1.0f);
    }
    catch (...) {
        return getRandomFValue();
    }
}
float unisonToMacro(const std::string& name, const std::string& value) {
    static const std::unordered_map<std::string, float> uniToMacroMap = {
        {"1", 0.00f}, {"2", 0.04f}, {"3", 0.11f}, {"4", 0.19f},
        {"5", 0.25f}, {"6", 0.33f}, {"7", 0.37f}, {"8", 0.44f},
        {"9", 0.54f}, {"10", 0.60f}, {"11", 0.65f}, {"12", 0.70f},
        {"13", 0.80f}, {"14", 0.85f}, {"15", 0.90f}, {"16", 1.00f}
    };
    try {
        auto it = uniToMacroMap.find(value);
        if (it != uniToMacroMap.end()) {
            return it->second;
        }
    }
    catch (...) {
    }
    return getRandomFValue();
}
float octToMidi(const std::string& name, const std::string& value) {
    static const std::unordered_map<std::string, float> octToMidiMap = {
        {"-4 Oct", 0.00f}, {"-3 Oct", 0.10f}, {"-2 Oct", 0.20f}, {"-1 Oct", 0.35f},
        {"0 Oct", 0.45f}, {"+1 Oct", 0.60f}, {"+2 Oct", 0.70f}, {"+3 Oct", 0.85f},
        {"+4 Oct", 1.00f},
        {"1 Oct", 0.60f}, {"2 Oct", 0.70f}, {"3 Oct", 0.85f},
        {"4 Oct", 1.00f}
    };
    try {
        auto it = octToMidiMap.find(value);
        if (it != octToMidiMap.end()) {
            return it->second;
        }
    }
    catch (...) {
    }
    return getRandomFValue();
}
float semiToMacro(const std::string& name, const std::string& value) {
    static const std::unordered_map<std::string, float> semiToMacroMap = {
        {"-12", 0.00f}, {"-11", 0.03f}, {"-10", 0.07f}, {"-9", 0.11f},
        {"-8", 0.15f}, {"-7", 0.20f}, {"-6", 0.26f}, {"-5", 0.30f},
        {"-4", 0.35f}, {"-3", 0.37f}, {"-2", 0.40f}, {"-1", 0.45f},
        {"0", 0.50f}, {"+1", 0.55f}, {"+2", 0.60f}, {"+3", 0.62f},
        {"+4", 0.65f}, {"+5", 0.70f}, {"+6", 0.75f}, {"+7", 0.80f},
        {"+8", 0.85f}, {"+9", 0.87f}, {"+10", 0.90f}, {"+11", 0.95f},
        {"+12", 1.00f},{"1", 0.55f}, {"2", 0.60f}, {"3", 0.62f},
        {"4", 0.65f}, {"5", 0.70f}, {"6", 0.75f}, {"7", 0.80f},
        {"8", 0.85f}, {"9", 0.87f}, {"10", 0.90f}, {"11", 0.95f},
        {"12", 1.00f}
    };
    try {
        std::string sanitizedValue = value;
        sanitizedValue.erase(std::remove(sanitizedValue.begin(), sanitizedValue.end(), ' '), sanitizedValue.end());
        sanitizedValue.erase(std::remove(sanitizedValue.begin(), sanitizedValue.end(), 's'), sanitizedValue.end());
        sanitizedValue.erase(std::remove(sanitizedValue.begin(), sanitizedValue.end(), 'e'), sanitizedValue.end());
        sanitizedValue.erase(std::remove(sanitizedValue.begin(), sanitizedValue.end(), 'm'), sanitizedValue.end());
        sanitizedValue.erase(std::remove(sanitizedValue.begin(), sanitizedValue.end(), 'i'), sanitizedValue.end());
        sanitizedValue.erase(std::remove(sanitizedValue.begin(), sanitizedValue.end(), 't'), sanitizedValue.end());
        sanitizedValue.erase(std::remove(sanitizedValue.begin(), sanitizedValue.end(), 'o'), sanitizedValue.end());
        sanitizedValue.erase(std::remove(sanitizedValue.begin(), sanitizedValue.end(), 'n'), sanitizedValue.end());
        auto it = semiToMacroMap.find(sanitizedValue);
        if (it != semiToMacroMap.end()) {
            return it->second;
        }
    }
    catch (...) {
    }
    return getRandomFValue();
}
float fineToMacro(const std::string& name, const std::string& value) {
    try {
        std::string sanitizedValue = value;
        sanitizedValue.erase(std::remove(sanitizedValue.begin(), sanitizedValue.end(), ' '), sanitizedValue.end());
        sanitizedValue.erase(std::remove(sanitizedValue.begin(), sanitizedValue.end(), 'c'), sanitizedValue.end());
        sanitizedValue.erase(std::remove(sanitizedValue.begin(), sanitizedValue.end(), 'e'), sanitizedValue.end());
        sanitizedValue.erase(std::remove(sanitizedValue.begin(), sanitizedValue.end(), 'n'), sanitizedValue.end());
        sanitizedValue.erase(std::remove(sanitizedValue.begin(), sanitizedValue.end(), 't'), sanitizedValue.end());
        sanitizedValue.erase(std::remove(sanitizedValue.begin(), sanitizedValue.end(), 's'), sanitizedValue.end());
        int cents = std::stoi(sanitizedValue);
        if (cents < -100 || cents > 100)
            return getRandomFValue();
        return (cents + 100.0f) / 200.0f;
    }
    catch (...) {
        return getRandomFValue();
    }
}
float frequencyToPercentage(const std::string& name, const std::string& value) {
    std::vector<std::pair<float, float>> freqPercentageMap = {
        {8, 0.00f}, {9, 0.0079f}, {10, 0.0236f}, {11, 0.0394f}, {12, 0.0472f}, {13, 0.0551f},
        {14, 0.0709f}, {15, 0.0866f}, {17, 0.0945f}, {18, 0.102f}, {20, 0.110f}, {21, 0.118f},
        {22, 0.126f}, {24, 0.134f}, {25, 0.142f}, {27, 0.150f}, {28, 0.157f}, {30, 0.165f},
        {32, 0.173f}, {34, 0.181f}, {36, 0.189f}, {39, 0.197f}, {41, 0.205f}, {44, 0.213f},
        {47, 0.220f}, {50, 0.228f}, {53, 0.236f}, {56, 0.244f}, {60, 0.252f}, {64, 0.260f},
        {68, 0.268f}, {72, 0.276f}, {77, 0.283f}, {82, 0.291f}, {87, 0.299f}, {93, 0.307f},
        {99, 0.315f}, {105, 0.323f}, {111, 0.331f}, {119, 0.339f}, {126, 0.346f}, {134, 0.354f},
        {143, 0.362f}, {158, 0.370f}, {162, 0.378f}, {172, 0.386f}, {184, 0.394f}, {195, 0.402f},
        {208, 0.409f}, {221, 0.417f}, {235, 0.425f}, {251, 0.433f}, {261, 0.441f}, {284, 0.449f},
        {302, 0.457f}, {321, 0.465f}, {342, 0.472f}, {364, 0.480f}, {387, 0.496f}, {438, 0.504f},
        {467, 0.512f}, {497, 0.520f}, {528, 0.528f}, {562, 0.535f}, {599, 0.543f}, {637, 0.551f},
        {678, 0.559f}, {722, 0.567f}, {768, 0.575f}, {816, 0.583f}, {869, 0.591f}, {925, 0.598f},
        {984, 0.606f}, {1047, 0.614f}, {1115, 0.622f}, {1186, 0.630f}, {1263, 0.638f}, {1344, 0.646f},
        {1430, 0.654f}, {1522, 0.661f}, {1620, 0.669f}, {1724, 0.677f}, {1835, 0.685f}, {1952, 0.693f},
        {2078, 0.701f}, {2209, 0.709f}, {2351, 0.717f}, {2503, 0.724f}, {2663, 0.732f}, {2834, 0.740f},
        {3017, 0.748f}, {3210, 0.756f}, {3417, 0.764f}, {3636, 0.772f}, {3870, 0.780f}, {4119, 0.787f},
        {4383, 0.795f}, {4665, 0.803f}, {4965, 0.811f}, {5284, 0.819f}, {5623, 0.827f}, {5979, 0.835f},
        {6363, 0.843f}, {6772, 0.850f}, {7207, 0.858f}, {7670, 0.866f}, {8163, 0.874f}, {8688, 0.882f},
        {9246, 0.890f}, {9840, 0.898f}, {10472, 0.906f}, {11145, 0.913f}, {11861, 0.921f}, {12623, 0.929f},
        {13434, 0.937f}, {14298, 0.945f}, {15216, 0.953f}, {16194, 0.961f}, {17219, 0.969f}, {18326, 0.976f},
        {19503, 0.984f}, {20756, 0.992f}, {22050, 1.000f}
    };
    try {
        std::regex re(R"(([+-]?[0-9]*\.?[0-9]+)\s*Hz)");
        std::smatch match;
        if (!std::regex_search(value, match, re))
            return getRandomFValue();
        float frequency = std::stof(match[1].str());
        for (size_t i = 1; i < freqPercentageMap.size(); i++) {
            if (frequency <= freqPercentageMap[i].first) {
                float f_low = freqPercentageMap[i - 1].first;
                float f_high = freqPercentageMap[i].first;
                float p_low = freqPercentageMap[i - 1].second;
                float p_high = freqPercentageMap[i].second;
                float interpolatedPercentage = p_low + (p_high - p_low) * ((frequency - f_low) / (f_high - f_low));
                return std::clamp(interpolatedPercentage, 0.0f, 1.0f);
            }
        }
        return 1.0f;
    }
    catch (...) {
        return getRandomFValue();
    }
}
float filterTypeToMacro(const std::string& name, const std::string& value) {
    static const std::unordered_map<std::string, float> filterPercentages = {
        {"MG Low 6", 0.00f}, {"MG Low 12", 0.0079f}, {"MG Low 18", 0.0236f}, {"MG Low 24", 0.0315f},
        {"Low 6", 0.0394f}, {"Low 12", 0.0551f}, {"Low 18", 0.0630f}, {"Low 24", 0.0709f},
        {"High 6", 0.0866f}, {"High 12", 0.0945f}, {"High 18", 0.102f}, {"High 24", 0.118f},
        {"Band 12", 0.126f}, {"Band 24", 0.134f}, {"Peak 12", 0.150f}, {"Peak 24", 0.157f},
        {"Notch 12", 0.165f}, {"Notch 24", 0.181f}, {"LH 6", 0.189f}, {"LH 12", 0.197f},
        {"LB 12", 0.213f}, {"LP 12", 0.220f}, {"LN 12", 0.228f}, {"HB 12", 0.244f},
        {"HP 12", 0.252f}, {"HN 12", 0.260f}, {"BP 12", 0.276f}, {"PP 12", 0.291f},
        {"PN 12", 0.307f}, {"NN 12", 0.315f}, {"L/B/H 12", 0.323f}, {"L/B/H 24", 0.339f},
        {"L/P/H 12", 0.346f}, {"L/P/H 24", 0.354f}, {"L/N/H 12", 0.370f}, {"L/N/H 24", 0.378f},
        {"B/P/N 12", 0.386f}, {"B/P/N 24", 0.402f}, {"Cmb +", 0.409f}, {"Cmb -", 0.417f},
        {"Cmb L6+", 0.433f}, {"Cmb L6-", 0.441f}, {"Cmb H6+", 0.449f}, {"Cmb H6-", 0.465f},
        {"Cmb HL6+", 0.472f}, {"Cmb HL6-", 0.480f}, {"Flg +", 0.496f}, {"Flg -", 0.504f},
        {"Flg L6+", 0.512f}, {"Flg L6-", 0.528f}, {"Flg H6+", 0.535f}, {"Flg H6-", 0.543f},
        {"Flg HL6+", 0.559f}, {"Flg HL6-", 0.567f}, {"Phs 12+", 0.575f}, {"Phs 12-", 0.591f},
        {"Phs 24+", 0.598f}, {"Phs 24-", 0.606f}, {"Phs 36+", 0.622f}, {"Phs 36-", 0.630f},
        {"Phs 48+", 0.638f}, {"Phs 48-", 0.654f}, {"Phs 48L6+", 0.661f}, {"Phs 48L6-", 0.669f},
        {"Phs 48H6+", 0.685f}, {"Phs 48H6-", 0.693f}, {"Phs 48HL6+", 0.701f}, {"Phs 48HL6-", 0.717f},
        {"FPhs 12HL6+", 0.724f}, {"FPhs 12HL6-", 0.732f}, {"Low EQ 6", 0.748f}, {"Low EQ 12", 0.756f},
        {"Band EQ 12", 0.764f}, {"High EQ 6", 0.780f}, {"High EQ 12", 0.787f}, {"Ring Mod", 0.795f},
        {"Ring Modx2", 0.811f}, {"SampHold", 0.819f}, {"SampHold-", 0.827f}, {"Combs", 0.843f},
        {"Allpasses", 0.850f}, {"Reverb", 0.858f}, {"French LP", 0.874f}, {"German LP", 0.882f},
        {"Add Bass", 0.890f}, {"Formant-I", 0.906f}, {"Formant-II", 0.913f}, {"Formant-III", 0.921f},
        {"Bandreject", 0.937f}, {"Dist.Comb 1 LP", 0.945f}, {"Dist.Comb 1 BP", 0.961f},
        {"Dist.Comb 2 LP", 0.969f}, {"Dist.Comb 2 BP", 0.976f}, {"Scream LP", 0.984f},
        {"Scream BP", 1.000f}
    };
    auto it = filterPercentages.find(value);
    if (it != filterPercentages.end()) {
        return it->second;
    }
    return getRandomFValue();
}

float distortionTypeToMacro(const std::string& name, const std::string& value) {
    static const std::unordered_map<std::string, float> distortionPercentages = {
        {"Tube", 0.00f}, {"SoftClip", 0.0394f}, {"HardClip", 0.102f}, {"Diode 1", 0.173f},
        {"Diode 2", 0.236f}, {"Lin.Fold", 0.307f}, {"Sin Fold", 0.370f}, {"Zero-Square", 0.441f},
        {"Downsample", 0.504f}, {"Asym", 0.567f}, {"Rectify", 0.638f}, {"X-Shaper", 0.701f},
        {"X-Shaper (Asym)", 0.772f}, {"Sine Shaper", 0.835f}, {"Stomp Box", 0.906f}, {"Tape Stop.", 1.000f}
    };
    auto it = distortionPercentages.find(value);
    if (it != distortionPercentages.end()) {
        return it->second;
    }
    return getRandomFValue();
}
float uniblendToF(const std::string& name, const std::string& value) {
    try {
        float blendValue = std::stof(value);
        if (blendValue < 0.0f || blendValue > 100.0f) {
            return getRandomFValue();
        }
        return blendValue / 100.0f;
    }
    catch (...) {
        return getRandomFValue();
    }
}
float subShapeToMacro(const std::string& name, const std::string& value) {
    static const std::unordered_map<std::string, float> subOscShapePercentages = {
        {"Sine", 0.00f}, {"RoundRect", 0.12f}, {"Triangle", 0.32f},
        {"Saw", 0.52f}, {"Square", 0.75f}, {"Pulse", 1.00f}
    };
    auto it = subOscShapePercentages.find(value);
    if (it != subOscShapePercentages.end()) {
        return it->second;
    }
    return getRandomFValue();
}
float onToPercentage(const std::string& name, const std::string& value) {
    static const std::unordered_map<std::string, float> onOffMap = {
        {"On", 0.51f}, {"ON", 0.51f}, {"on", 0.51f}, {"retrig", 0.51f}, {"link", 0.51f},
        {"LINK", 0.51f}, {"Link", 0.51f}, {"1", 0.51f},

        {"Off", 0.00f}, {"off", 0.00f}, {"OFF", 0.00f}, {"Unlink", 0.00f}, {"unlink", 0.00f},
        {"UNLINK", 0.00f}, {"0", 0.00f}
    };
    auto it = onOffMap.find(value);
    if (it != onOffMap.end()) {
        return it->second;
    }
    try {
        int intValue = std::stoi(value);
        return intValue == 1 ? 0.50f : 0.00f;
    }
    catch (...) {
        return getRandomFValue();
    }
}

float hypUnisonToMacro(const std::string& name, const std::string& value) {
    static const std::unordered_map<int, float> hypUnisonMap = {
        {0, 0.00f},  {1, 0.14f}, {2, 0.28f}, {3, 0.42f},
        {4, 0.56f},  {5, 0.70f}, {6, 0.85f}, {7, 1.00f}
    };
    try {
        int intValue = std::stoi(value);
        auto it = hypUnisonMap.find(intValue);
        return (it != hypUnisonMap.end()) ? it->second : getRandomFValue();
    }
    catch (...) {
        return getRandomFValue();
    }
}
float uniDetToMacro(const std::string& name, const std::string& value) {
    try {
        float intensity = std::stof(value);
        if (intensity < 0.0f || intensity > 1.0f)
            return getRandomFValue();
        return std::sqrt(intensity);
    }
    catch (...) {
        return getRandomFValue();
    }
}
float distPrePostToMacro(const std::string& name, const std::string& value) {
    static const std::unordered_map<std::string, float> settings = {
        {"Off", 0.08f}, {"off", 0.08f}, {"OFF", 0.08f}, {"0", 0.0f},
        {"Pre", 0.35f}, {"pre", 0.35f}, {"PRE", 0.35f},
        {"Post", 1.00f}, {"post", 1.00f}, {"POST", 1.00f}
    };
    auto it = settings.find(value);
    if (it != settings.end()) {
        return it->second;
    }
    return getRandomFValue();
}
float distBwToPercentage(const std::string& name, const std::string& value) {
    try {
        float inputValue = std::stof(value);
        if (inputValue < 0.1f) return 0.0f;
        if (inputValue > 7.6f) return 1.0f;
        if (inputValue <= 0.5f)
            return (10.0f + (inputValue - 0.1f) / (0.5f - 0.1f) * (25.0f - 10.0f)) / 100.0f;
        else if (inputValue <= 1.9f)
            return (25.0f + (inputValue - 0.5f) / (1.9f - 0.5f) * (50.0f - 25.0f)) / 100.0f;
        else if (inputValue <= 3.0f)
            return (50.0f + (inputValue - 1.9f) / (3.0f - 1.9f) * (62.0f - 50.0f)) / 100.0f;
        else if (inputValue <= 3.6f)
            return (62.0f + (inputValue - 3.0f) / (3.6f - 3.0f) * (68.9f - 62.0f)) / 100.0f;
        else if (inputValue <= 4.3f)
            return (68.9f + (inputValue - 3.6f) / (4.3f - 3.6f) * (75.0f - 68.9f)) / 100.0f;
        else if (inputValue <= 7.6f)
            return (75.0f + (inputValue - 4.3f) / (7.6f - 4.3f) * (100.0f - 75.0f)) / 100.0f;
    }
    catch (...) {
        return getRandomFValue();
    }
    return getRandomFValue();
}
float phaseRateToMacro(const std::string& name, const std::string& value) {
    try {
        float rate = std::stof(value);
        if (rate < 0.0f || rate > 20.0f) return getRandomFValue();
        if (rate <= 0.03f)
            return (rate / 0.03f) * 0.20f;
        else if (rate <= 0.16f)
            return (0.20f + (rate - 0.03f) / (0.16f - 0.03f) * (0.30f - 0.20f));
        else if (rate <= 0.51f)
            return (0.30f + (rate - 0.16f) / (0.51f - 0.16f) * (0.40f - 0.30f));
        else if (rate <= 1.25f)
            return (0.40f + (rate - 0.51f) / (1.25f - 0.51f) * (0.50f - 0.40f));
        else if (rate <= 2.59f)
            return (0.50f + (rate - 1.25f) / (2.59f - 1.25f) * (0.60f - 0.50f));
        else if (rate <= 4.80f)
            return (0.60f + (rate - 2.59f) / (4.80f - 2.59f) * (0.70f - 0.60f));
        else if (rate <= 8.19f)
            return (0.70f + (rate - 4.80f) / (8.19f - 4.80f) * (0.80f - 0.70f));
        else if (rate <= 13.12f)
            return (0.80f + (rate - 8.19f) / (13.12f - 8.19f) * (0.90f - 0.80f));
        else if (rate <= 20.0f)
            return (0.90f + (rate - 13.12f) / (20.0f - 13.12f) * (1.00f - 0.90f));
    }
    catch (...) {
        return getRandomFValue();
    }
    return getRandomFValue();
}
float degreesToPercentage(const std::string& name, const std::string& value) {
    try {
        float degrees = std::stof(value);
        if (degrees < 0.0f || degrees > 360.0f) return getRandomFValue();
        return degrees / 360.0f;
    }
    catch (...) {
        return getRandomFValue();
    }
}
float choDlyToPercentage(const std::string& name, const std::string& value) {
    try {
        float msValue = std::stof(value);

        if (msValue < 0.0f || msValue > 20.0f) return getRandomFValue();

        if (msValue <= 0.2f)
            return (msValue / 0.2f) * 0.1f;
        else if (msValue <= 0.8f)
            return 0.1f + ((msValue - 0.2f) / (0.8f - 0.2f)) * (0.2f - 0.1f);
        else if (msValue <= 1.8f)
            return 0.2f + ((msValue - 0.8f) / (1.8f - 0.8f)) * (0.3f - 0.2f);
        else if (msValue <= 3.2f)
            return 0.3f + ((msValue - 1.8f) / (3.2f - 1.8f)) * (0.4f - 0.3f);
        else if (msValue <= 5.0f)
            return 0.4f + ((msValue - 3.2f) / (5.0f - 3.2f)) * (0.5f - 0.4f);
        else if (msValue <= 7.2f)
            return 0.5f + ((msValue - 5.0f) / (7.2f - 5.0f)) * (0.6f - 0.5f);
        else if (msValue <= 9.8f)
            return 0.6f + ((msValue - 7.2f) / (9.8f - 7.2f)) * (0.7f - 0.6f);
        else if (msValue <= 12.8f)
            return 0.7f + ((msValue - 9.8f) / (12.8f - 9.8f)) * (0.8f - 0.7f);
        else if (msValue <= 16.2f)
            return 0.8f + ((msValue - 12.8f) / (16.2f - 12.8f)) * (0.9f - 0.8f);
        else
            return 0.9f + ((msValue - 16.2f) / (20.0f - 16.2f)) * (1.0f - 0.9f);
    }
    catch (...) {
        return getRandomFValue();
    }
}
float choDepToPercentage(const std::string& name, const std::string& value) {
    try {
        float msValue = std::stof(value);
        if (msValue < 0.0f || msValue > 26.0f) return getRandomFValue();
        if (msValue <= 0.3f)
            return (msValue / 0.3f) * 0.1f;
        else if (msValue <= 1.0f)
            return 0.1f + ((msValue - 0.3f) / (1.0f - 0.3f)) * (0.2f - 0.1f);
        else if (msValue <= 2.3f)
            return 0.2f + ((msValue - 1.0f) / (2.3f - 1.0f)) * (0.3f - 0.2f);
        else if (msValue <= 4.2f)
            return 0.3f + ((msValue - 2.3f) / (4.2f - 2.3f)) * (0.4f - 0.3f);
        else if (msValue <= 6.5f)
            return 0.4f + ((msValue - 4.2f) / (6.5f - 4.2f)) * (0.5f - 0.4f);
        else if (msValue <= 9.4f)
            return 0.5f + ((msValue - 6.5f) / (9.4f - 6.5f)) * (0.6f - 0.5f);
        else if (msValue <= 12.7f)
            return 0.6f + ((msValue - 9.4f) / (12.7f - 9.4f)) * (0.7f - 0.6f);
        else if (msValue <= 16.6f)
            return 0.7f + ((msValue - 12.7f) / (16.6f - 12.7f)) * (0.8f - 0.7f);
        else if (msValue <= 21.1f)
            return 0.8f + ((msValue - 16.6f) / (21.1f - 16.6f)) * (0.9f - 0.8f);
        else
            return 0.9f + ((msValue - 21.1f) / (26.0f - 21.1f)) * (1.0f - 0.9f);
    }
    catch (...) {
        return getRandomFValue();
    }
}
float choFiltToPercentage(const std::string& name, const std::string& value) {
    try {
        float freq = std::stof(value);
        if (freq < 50.0f || freq > 20000.0f) return getRandomFValue();
        if (freq <= 91.0f)
            return (freq - 50.0f) / (91.0f - 50.0f) * 0.1f;
        else if (freq <= 166.0f)
            return 0.1f + ((freq - 91.0f) / (166.0f - 91.0f)) * (0.2f - 0.1f);
        else if (freq <= 302.0f)
            return 0.2f + ((freq - 166.0f) / (302.0f - 166.0f)) * (0.3f - 0.2f);
        else if (freq <= 549.0f)
            return 0.3f + ((freq - 302.0f) / (549.0f - 302.0f)) * (0.4f - 0.3f);
        else if (freq <= 1000.0f)
            return 0.4f + ((freq - 549.0f) / (1000.0f - 549.0f)) * (0.5f - 0.4f);
        else if (freq <= 1821.0f)
            return 0.5f + ((freq - 1000.0f) / (1821.0f - 1000.0f)) * (0.6f - 0.5f);
        else if (freq <= 3314.0f)
            return 0.6f + ((freq - 1821.0f) / (3314.0f - 1821.0f)) * (0.7f - 0.6f);
        else if (freq <= 6034.0f)
            return 0.7f + ((freq - 3314.0f) / (6034.0f - 3314.0f)) * (0.8f - 0.7f);
        else if (freq <= 10986.0f)
            return 0.8f + ((freq - 6034.0f) / (10986.0f - 6034.0f)) * (0.9f - 0.8f);
        else
            return 0.9f + ((freq - 10986.0f) / (20000.0f - 10986.0f)) * (1.0f - 0.9f);
    }
    catch (...) {
        return getRandomFValue();
    }
}
float distFreqToPercentage(const std::string& name, const std::string& value) {
    try {
        float freq = std::stof(value);
        if (freq < 8.0f) return 0.0f;
        if (freq > 13290.0f) return 1.0f;
        if (freq <= 17.0f)
            return (freq - 8.0f) / (17.0f - 8.0f) * 0.1f;
        else if (freq <= 36.0f)
            return 0.1f + ((freq - 17.0f) / (36.0f - 17.0f)) * (0.2f - 0.1f);
        else if (freq <= 75.0f)
            return 0.2f + ((freq - 36.0f) / (75.0f - 36.0f)) * (0.3f - 0.2f);
        else if (freq <= 157.0f)
            return 0.3f + ((freq - 75.0f) / (157.0f - 75.0f)) * (0.4f - 0.3f);
        else if (freq <= 330.0f)
            return 0.4f + ((freq - 157.0f) / (330.0f - 157.0f)) * (0.5f - 0.4f);
        else if (freq <= 690.0f)
            return 0.5f + ((freq - 330.0f) / (690.0f - 330.0f)) * (0.6f - 0.5f);
        else if (freq <= 1446.0f)
            return 0.6f + ((freq - 690.0f) / (1446.0f - 690.0f)) * (0.7f - 0.6f);
        else if (freq <= 3030.0f)
            return 0.7f + ((freq - 1446.0f) / (3030.0f - 1446.0f)) * (0.8f - 0.7f);
        else if (freq <= 6346.0f)
            return 0.8f + ((freq - 3030.0f) / (6346.0f - 3030.0f)) * (0.9f - 0.8f);
        else
            return 0.9f + ((freq - 6346.0f) / (13290.0f - 6346.0f)) * (1.0f - 0.9f);
    }
    catch (...) {
        return getRandomFValue();
    }
}
float dlyFreqToPercentage(const std::string& name, const std::string& value) {
    try {
        float freq = std::stof(value);
        if (freq < 40.0f) return 0.0f;
        if (freq > 18000.0f) return 1.0f;
        if (freq <= 74.0f)
            return (freq - 40.0f) / (74.0f - 40.0f) * 0.1f;
        else if (freq <= 136.0f)
            return 0.1f + ((freq - 74.0f) / (136.0f - 74.0f)) * (0.2f - 0.1f);
        else if (freq <= 250.0f)
            return 0.2f + ((freq - 136.0f) / (250.0f - 136.0f)) * (0.3f - 0.2f);
        else if (freq <= 461.0f)
            return 0.3f + ((freq - 250.0f) / (461.0f - 250.0f)) * (0.4f - 0.3f);
        else if (freq <= 849.0f)
            return 0.4f + ((freq - 461.0f) / (849.0f - 461.0f)) * (0.5f - 0.4f);
        else if (freq <= 1563.0f)
            return 0.5f + ((freq - 849.0f) / (1563.0f - 849.0f)) * (0.6f - 0.5f);
        else if (freq <= 2879.0f)
            return 0.6f + ((freq - 1563.0f) / (2879.0f - 1563.0f)) * (0.7f - 0.6f);
        else if (freq <= 5304.0f)
            return 0.7f + ((freq - 2879.0f) / (5304.0f - 2879.0f)) * (0.8f - 0.7f);
        else if (freq <= 9771.0f)
            return 0.8f + ((freq - 5304.0f) / (9771.0f - 5304.0f)) * (0.9f - 0.8f);
        else
            return 0.9f + ((freq - 9771.0f) / (18000.0f - 9771.0f)) * (1.0f - 0.9f);
    }
    catch (...) {
        return getRandomFValue();
    }
}
float phsFrqToPercentage(const std::string& name, const std::string& value) {
    try {
        float freq = std::stof(value);
        if (freq < 20.0f) return 0.0f;
        if (freq > 18000.0f) return 1.0f;
        if (freq <= 39.0f)
            return (freq - 20.0f) / (39.0f - 20.0f) * 0.1f;
        else if (freq <= 77.0f)
            return 0.1f + ((freq - 39.0f) / (77.0f - 39.0f)) * (0.2f - 0.1f);
        else if (freq <= 153.0f)
            return 0.2f + ((freq - 77.0f) / (153.0f - 77.0f)) * (0.3f - 0.2f);
        else if (freq <= 303.0f)
            return 0.3f + ((freq - 153.0f) / (303.0f - 153.0f)) * (0.4f - 0.3f);
        else if (freq <= 600.0f)
            return 0.4f + ((freq - 303.0f) / (600.0f - 303.0f)) * (0.5f - 0.4f);
        else if (freq <= 1184.0f)
            return 0.5f + ((freq - 600.0f) / (1184.0f - 600.0f)) * (0.6f - 0.5f);
        else if (freq <= 2338.0f)
            return 0.6f + ((freq - 1184.0f) / (2338.0f - 1184.0f)) * (0.7f - 0.6f);
        else if (freq <= 4617.0f)
            return 0.7f + ((freq - 2338.0f) / (4617.0f - 2338.0f)) * (0.8f - 0.7f);
        else if (freq <= 9116.0f)
            return 0.8f + ((freq - 4617.0f) / (9116.0f - 4617.0f)) * (0.9f - 0.8f);
        else
            return 0.9f + ((freq - 9116.0f) / (18000.0f - 9116.0f)) * (1.0f - 0.9f);
    }
    catch (...) {
        return getRandomFValue();
    }
}
float EQfrqToPercentage(const std::string& name, const std::string& value) {
    try {
        float freq = std::stof(value);
        if (freq < 22.0f) return 0.0f;
        if (freq > 20000.0f) return 1.0f;
        if (freq <= 43.0f)
            return (freq - 22.0f) / (43.0f - 22.0f) * 0.1f;
        else if (freq <= 84.0f)
            return 0.1f + ((freq - 43.0f) / (84.0f - 43.0f)) * (0.2f - 0.1f);
        else if (freq <= 167.0f)
            return 0.2f + ((freq - 84.0f) / (167.0f - 84.0f)) * (0.3f - 0.2f);
        else if (freq <= 331.0f)
            return 0.3f + ((freq - 167.0f) / (331.0f - 167.0f)) * (0.4f - 0.3f);
        else if (freq <= 656.0f)
            return 0.4f + ((freq - 331.0f) / (656.0f - 331.0f)) * (0.5f - 0.4f);
        else if (freq <= 1300.0f)
            return 0.5f + ((freq - 656.0f) / (1300.0f - 656.0f)) * (0.6f - 0.5f);
        else if (freq <= 2574.0f)
            return 0.6f + ((freq - 1300.0f) / (2574.0f - 1300.0f)) * (0.7f - 0.6f);
        else if (freq <= 5099.0f)
            return 0.7f + ((freq - 2574.0f) / (5099.0f - 2574.0f)) * (0.8f - 0.7f);
        else if (freq <= 10098.0f)
            return 0.8f + ((freq - 5099.0f) / (10098.0f - 5099.0f)) * (0.9f - 0.8f);
        else
            return 0.9f + ((freq - 10098.0f) / (20000.0f - 10098.0f)) * (1.0f - 0.9f);
    }
    catch (...) {
        return getRandomFValue();
    }
}
float dlyBwToPercentage(const std::string& name, const std::string& value) {
    try {
        float val = std::stof(value);
        if (val < 0.8f) return 0.0f;
        if (val > 8.2f) return 1.0f;
        if (val <= 1.5f)
            return (val - 0.8f) / (1.5f - 0.8f) * 0.1f;
        else if (val <= 2.2f)
            return 0.1f + ((val - 1.5f) / (2.2f - 1.5f)) * (0.2f - 0.1f);
        else if (val <= 3.0f)
            return 0.2f + ((val - 2.2f) / (3.0f - 2.2f)) * (0.3f - 0.2f);
        else if (val <= 3.8f)
            return 0.3f + ((val - 3.0f) / (3.8f - 3.0f)) * (0.4f - 0.3f);
        else if (val <= 4.5f)
            return 0.4f + ((val - 3.8f) / (4.5f - 3.8f)) * (0.5f - 0.4f);
        else if (val <= 5.3f)
            return 0.5f + ((val - 4.5f) / (5.3f - 4.5f)) * (0.6f - 0.5f);
        else if (val <= 6.0f)
            return 0.6f + ((val - 5.3f) / (6.0f - 5.3f)) * (0.7f - 0.6f);
        else if (val <= 6.8f)
            return 0.7f + ((val - 6.0f) / (6.8f - 6.0f)) * (0.8f - 0.7f);
        else if (val <= 7.5f)
            return 0.8f + ((val - 6.8f) / (7.5f - 6.8f)) * (0.9f - 0.8f);
        else
            return 0.9f + ((val - 7.5f) / (8.2f - 7.5f)) * (1.0f - 0.9f);
    }
    catch (...) {
        return getRandomFValue();
    }
}
float dlyModeToPercentage(const std::string& name, const std::string& value) {
    static const std::unordered_map<std::string, float> dlyModeMap = {
        {"Normal", 0.18f}, {"0", 0.18f}, {"normal", 0.18f}, {"NORMAL", 0.18f},
        {"Ping-Pong", 0.30f}, {"ping pong", 0.30f}, {"pingpong", 0.30f},
        {"PingPong", 0.30f}, {"Ping Pong", 0.30f}, {"1", 0.30f}, {"ping-pong", 0.30f}, {"Ping-pong", 0.30f}, {"ping-Pong", 0.30f},
        {"Tap->Delay", 0.80f}, {"2", 0.80f}, {"tapdelay", 0.80f},
        {"TapDelay", 0.80f}, {"tap delay", 0.80f}, {"Tap Delay", 0.80f}
    };
    auto it = dlyModeMap.find(value);
    if (it != dlyModeMap.end()) {
        return it->second;
    }
    return getRandomFValue();
}
float delayTimeToPercentage(const std::string& name, const std::string& value) {
    static const std::unordered_map<std::string, float> beatMappings = {
        {"fast", 0.0f}, {"1/256", 0.0709f}, {"1/128", 0.197f}, {"1/64", 0.252f},
        {"1/32", 0.346f}, {"1/16", 0.480f}, {"1/8", 0.551f}, {"1/4", 0.606f},
        {"1/2", 0.764f}, {"Bar", 0.780f}, {"2 Bar", 0.890f}, {"4 Bar", 1.000f}
    };
    auto it = beatMappings.find(value);
    if (it != beatMappings.end())
        return it->second;
    float delayTime;
    try {
        std::string valueStr = value;
        valueStr.erase(std::remove(valueStr.begin(), valueStr.end(), 'm'), valueStr.end());
        std::stringstream ss(valueStr);
        ss >> delayTime;
        if (ss.fail()) return getRandomFValue();
    }
    catch (...) {
        return getRandomFValue();
    }
    if (delayTime < 1.0f) return 0.0f;
    if (delayTime > 501.0f) return 1.0f;
    if (1.0f <= delayTime && delayTime <= 1.05f)
        return (delayTime - 1.0f) / (1.05f - 1.0f) * 0.1f;
    else if (1.05f < delayTime && delayTime <= 1.8f)
        return 0.1f + (delayTime - 1.05f) / (1.8f - 1.05f) * (0.2f - 0.1f);
    else if (1.8f < delayTime && delayTime <= 5.05f)
        return 0.2f + (delayTime - 1.8f) / (5.05f - 1.8f) * (0.3f - 0.2f);
    else if (5.05f < delayTime && delayTime <= 13.8f)
        return 0.3f + (delayTime - 5.05f) / (13.8f - 5.05f) * (0.4f - 0.3f);
    else if (13.8f < delayTime && delayTime <= 32.25f)
        return 0.4f + (delayTime - 13.8f) / (32.25f - 13.8f) * (0.5f - 0.4f);
    else if (32.25f < delayTime && delayTime <= 65.8f)
        return 0.5f + (delayTime - 32.25f) / (65.8f - 32.25f) * (0.6f - 0.5f);
    else if (65.8f < delayTime && delayTime <= 121.05f)
        return 0.6f + (delayTime - 65.8f) / (121.05f - 65.8f) * (0.7f - 0.6f);
    else if (121.05f < delayTime && delayTime <= 205.8f)
        return 0.7f + (delayTime - 121.05f) / (205.8f - 121.05f) * (0.8f - 0.7f);
    else if (205.8f < delayTime && delayTime <= 329.05f)
        return 0.8f + (delayTime - 205.8f) / (329.05f - 205.8f) * (0.9f - 0.8f);
    else if (329.05f < delayTime && delayTime <= 501.0f)
        return 0.9f + (delayTime - 329.05f) / (501.0f - 329.05f) * (1.0f - 0.9f);
    return getRandomFValue();
}
float cmpThrToPercentage(const std::string& name, const std::string& value) {
    float threshold;
    try {
        std::string valueStr = value;
        valueStr.erase(std::remove(valueStr.begin(), valueStr.end(), 'd'), valueStr.end());
        std::stringstream ss(valueStr);
        ss >> threshold;
        if (ss.fail()) return getRandomFValue();
    }
    catch (...) {
        return getRandomFValue();
    }
    if (threshold > 0.0f) return 0.0f;
    if (threshold < -120.0f) return 1.0f;
    if (-120.0f <= threshold && threshold <= -60.0f)
        return 0.9f + (threshold + 60.0f) / (-120.0f + 60.0f) * (1.0f - 0.9f);
    else if (-60.0f < threshold && threshold <= -41.9f)
        return 0.8f + (threshold + 41.9f) / (-60.0f + 41.9f) * (0.9f - 0.8f);
    else if (-41.9f < threshold && threshold <= -31.4f)
        return 0.7f + (threshold + 31.4f) / (-41.9f + 31.4f) * (0.8f - 0.7f);
    else if (-31.4f < threshold && threshold <= -23.9f)
        return 0.6f + (threshold + 23.9f) / (-31.4f + 23.9f) * (0.7f - 0.6f);
    else if (-23.9f < threshold && threshold <= -18.1f)
        return 0.5f + (threshold + 18.1f) / (-23.9f + 18.1f) * (0.6f - 0.5f);
    else if (-18.1f < threshold && threshold <= -13.3f)
        return 0.4f + (threshold + 13.3f) / (-18.1f + 13.3f) * (0.5f - 0.4f);
    else if (-13.3f < threshold && threshold <= -9.3f)
        return 0.3f + (threshold + 9.3f) / (-13.3f + 9.3f) * (0.4f - 0.3f);
    else if (-9.3f < threshold && threshold <= -5.8f)
        return 0.2f + (threshold + 5.8f) / (-9.3f + 5.8f) * (0.3f - 0.2f);
    else if (-5.8f < threshold && threshold <= -2.7f)
        return 0.1f + (threshold + 2.7f) / (-5.8f + 2.7f) * (0.2f - 0.1f);
    else if (-2.7f < threshold && threshold <= 0.0f)
        return (threshold / -2.7f) * 0.1f;
    return getRandomFValue();
}
float cmpAttToPercentage(const std::string& name, const std::string& value) {
    try {
        float attack_time = std::stof(value);
        if (attack_time < 0.1f) return 0.0f;
        if (attack_time > 1000.0f) return 1.0f;
        if (attack_time <= 10.1f)
            return (attack_time - 0.1f) / (10.1f - 0.1f) * 0.1f;
        else if (attack_time <= 40.1f)
            return 0.1f + (attack_time - 10.1f) / (40.1f - 10.1f) * 0.1f;
        else if (attack_time <= 90.1f)
            return 0.2f + (attack_time - 40.1f) / (90.1f - 40.1f) * 0.1f;
        else if (attack_time <= 160.1f)
            return 0.3f + (attack_time - 90.1f) / (160.1f - 90.1f) * 0.1f;
        else if (attack_time <= 250.1f)
            return 0.4f + (attack_time - 160.1f) / (250.1f - 160.1f) * 0.1f;
        else if (attack_time <= 360.1f)
            return 0.5f + (attack_time - 250.1f) / (360.1f - 250.1f) * 0.1f;
        else if (attack_time <= 490.1f)
            return 0.6f + (attack_time - 360.1f) / (490.1f - 360.1f) * 0.1f;
        else if (attack_time <= 640.0f)
            return 0.7f + (attack_time - 490.1f) / (640.0f - 490.1f) * 0.1f;
        else if (attack_time <= 810.0f)
            return 0.8f + (attack_time - 640.0f) / (810.0f - 640.0f) * 0.1f;
        else if (attack_time <= 1000.0f)
            return 0.9f + (attack_time - 810.0f) / (1000.0f - 810.0f) * 0.1f;
    }
    catch (...) {
        return getRandomFValue();
    }

    return getRandomFValue();
}
float cmpGainToPercentage(const std::string& name, const std::string& value) {
    try {
        float gain = std::stof(value);
        if (gain < 0.0f) return 0.0f;
        if (gain > 30.1f) return 1.0f;
        if (gain <= 2.3f)
            return (gain / 2.3f) * 0.1f;
        else if (gain <= 7.0f)
            return 0.1f + (gain - 2.3f) / (7.0f - 2.3f) * 0.1f;
        else if (gain <= 11.6f)
            return 0.2f + (gain - 7.0f) / (11.6f - 7.0f) * 0.1f;
        else if (gain <= 15.5f)
            return 0.3f + (gain - 11.6f) / (15.5f - 11.6f) * 0.1f;
        else if (gain <= 18.8f)
            return 0.4f + (gain - 15.5f) / (18.8f - 15.5f) * 0.1f;
        else if (gain <= 21.7f)
            return 0.5f + (gain - 18.8f) / (21.7f - 18.8f) * 0.1f;
        else if (gain <= 24.7f)
            return 0.6f + (gain - 21.7f) / (24.7f - 21.7f) * 0.1f;
        else if (gain <= 26.4f)
            return 0.7f + (gain - 24.7f) / (26.4f - 24.7f) * 0.1f;
        else if (gain <= 28.3f)
            return 0.8f + (gain - 26.4f) / (28.3f - 26.4f) * 0.1f;
        else if (gain <= 30.1f)
            return 0.9f + (gain - 28.3f) / (30.1f - 28.3f) * 0.1f;
    }
    catch (...) {
        return getRandomFValue();
    }
    return getRandomFValue();
}
float CmpMBndToPercentage(const std::string& name, const std::string& value) {
    static const std::unordered_map<std::string, float> mapping = {
        {"Multiband", 1.0f}, {"multiband", 1.0f}, {"mb", 1.0f},
        {"MULTIBAND", 1.0f}, {"MB", 1.0f}, {"MultiBand", 1.0f},
        {"1", 1.0f}, {"Normal", 0.0f}, {"normal", 0.0f},
        {"NORMAL", 0.0f}, {"0", 0.0f}
    };
    auto it = mapping.find(value);
    if (it != mapping.end()) {
        return it->second;
    }
    return getRandomFValue();
}
float eqVolToPercentage(const std::string& name, const std::string& value) {
    try {
        float gain = std::stof(value.substr(0, value.find(" dB")));
        if (gain < -24.0f) return 0.0f;
        if (gain > 24.0f) return 1.0f;
        return (gain + 24.0f) / 48.0f;
    }
    catch (...) {
        return getRandomFValue();
    }
}
float eqTypToPercentage(const std::string& name, const std::string& value) {
    static const std::unordered_map<std::string, float> eqTypeMap = {
        {"Shelf", 0.18f}, {"shelf", 0.18f}, {"0", 0.18f},
        {"Peak", 0.30f}, {"peak", 0.30f}, {"PEAK", 0.30f}, {"1", 0.30f},
        {"LPF", 0.80f}, {"LP", 0.80f}, {"lpf", 0.80f}, {"lowpass", 0.80f}, {"Lpf", 0.80f}, {"2", 0.80f}
    };
    auto it = eqTypeMap.find(value);
    if (it != eqTypeMap.end()) {
        return it->second;
    }
    return getRandomFValue();
}
float decayToF(const std::string& name, const std::string& value) {
    try {
        std::string trimmedValue = value;
        if (trimmedValue.find(" s") != std::string::npos) {
            trimmedValue.erase(trimmedValue.find(" s"), 2);
        }
        float decayValue = std::stof(trimmedValue);
        if (decayValue < 0.8f || decayValue > 12.0f) {
            return getRandomFValue();
        }
        return (decayValue - 0.8f) / (12.0f - 0.8f);
    }
    catch (...) {
        return getRandomFValue();
    }
}
//might not need
float normalizePercentage(const std::string& name, const std::string& value) {
    std::regex re(R"(([+-]?[0-9]*\.?[0-9]+)\s*%)");
    std::smatch match;
    if (!std::regex_search(value, match, re))
        return static_cast<float>(rand() % 100) / 100.0f;
    float number = std::stof(match[1].str());
    return std::clamp(number / 100.0f, 0.0f, 1.0f);
}
float normalizeFrequency(const std::string& name, const std::string& value) {
    std::regex re(R"(([+-]?[0-9]*\.?[0-9]+)\s*Hz)");
    std::smatch match;
    if (!std::regex_search(value, match, re))
        return static_cast<float>(rand() % 127) / 127.0f;
    float number = std::stof(match[1].str());
    auto it = std::lower_bound(frequencies.begin(), frequencies.end(), number);
    int index = std::distance(frequencies.begin(), it);
    return std::clamp(static_cast<float>(index) / 127.0f, 0.0f, 1.0f);
}
float unisonToMidi(const std::string& name, const std::string& value) {
    return 0.0f;
}
float octToMacro(const std::string& name, const std::string& value) {
    return 0.0f;
}
float cmpRelToPercentage(const std::string& name, const std::string& value) {
    return 0.0f;
}
