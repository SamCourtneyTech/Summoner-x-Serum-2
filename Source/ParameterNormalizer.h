#pragma once
#ifndef PARAMETER_NORMALIZER_H
#define PARAMETER_NORMALIZER_H
#include <string>
#include <unordered_map>
#include <functional>
std::pair<std::string, float> normalizeValue(const std::string& paramName, const std::string& rawValue);
float normalizeMsS(const std::string& name, const std::string& value);
float normalizeDbToF(const std::string& name, const std::string& value);
float normalizePanToF(const std::string& name, const std::string& value);
float choDepthToPercentage(const std::string& name, const std::string& value);
float wtToMidi(const std::string& name, const std::string& value);
float percentageToMacro(const std::string& name, const std::string& value);
float octToMidi(const std::string& name, const std::string& value);
float choFeedToMacro(const std::string& name, const std::string& value);
float unisonToMacro(const std::string& name, const std::string& value);
float unisonToMidi(const std::string& name, const std::string& value);
float semiToMacro(const std::string& name, const std::string& value);
float fineToMacro(const std::string& name, const std::string& value);
float frequencyToPercentage(const std::string& name, const std::string& value);
float filterTypeToMacro(const std::string& name, const std::string& value);
float distortionTypeToMacro(const std::string& name, const std::string& value);
float uniblendToF(const std::string& name, const std::string& value);
float subShapeToMacro(const std::string& name, const std::string& value);
float onToPercentage(const std::string& name, const std::string& value);
float hypUnisonToMacro(const std::string& name, const std::string& value);
float uniDetToMacro(const std::string& name, const std::string& value);
float distPrePostToMacro(const std::string& name, const std::string& value);
float distBwToPercentage(const std::string& name, const std::string& value);
float phaseRateToMacro(const std::string& name, const std::string& value);
float degreesToPercentage(const std::string& name, const std::string& value);
float choDlyToPercentage(const std::string& name, const std::string& value);
float choDepToPercentage(const std::string& name, const std::string& value);
float choFiltToPercentage(const std::string& name, const std::string& value);
float distFreqToPercentage(const std::string& name, const std::string& value);
float dlyFreqToPercentage(const std::string& name, const std::string& value);
float phsFrqToPercentage(const std::string& name, const std::string& value);
float EQfrqToPercentage(const std::string& name, const std::string& value);
float delayTimeToPercentage(const std::string& name, const std::string& value);
float dlyBwToPercentage(const std::string& name, const std::string& value);
float dlyModeToPercentage(const std::string& name, const std::string& value);
float cmpThrToPercentage(const std::string& name, const std::string& value);
float cmpAttToPercentage(const std::string& name, const std::string& value);
float cmpRelToPercentage(const std::string& name, const std::string& value);
float cmpGainToPercentage(const std::string& name, const std::string& value);
float CmpMBndToPercentage(const std::string& name, const std::string& value);
float eqVolToPercentage(const std::string& name, const std::string& value);
float eqTypToPercentage(const std::string& name, const std::string& value);
float decayToF(const std::string& name, const std::string& value);
float normalizePercentage(const std::string& name, const std::string& value);
float normalizeFrequency(const std::string& name, const std::string& value);
float octToMacro(const std::string& name, const std::string& value);
float getRandomFValue();
#endif 
