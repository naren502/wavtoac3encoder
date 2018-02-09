﻿#pragma once

#include "StdAfx.h"
#include "utilities\StringHelper.h"
#include "utilities\Utilities.h"
#include "Configuration.h"

namespace config
{
    CConfiguration m_Config;

    const std::wstring szReadMode { L"rt, ccs=UTF-8" };
    const std::wstring szWriteMode { L"wt, ccs=UTF-8" };
    wchar_t Separator { '=' };

    const std::wstring szSeparator = L"=";
    const std::wstring szNewCharVar = L"\\n";
    const std::wstring szNewChar = L"\n";
    const std::wstring szTabCharVar = L"\\t";
    const std::wstring szTabChar = L"\t";

    std::wstring ReadAllText(const std::wstring& szFileName)
    {
        std::wstring buffer;

        FILE *fs;
        errno_t error = _wfopen_s(&fs, szFileName.c_str(), szReadMode.c_str());
        if (error != 0)
            return buffer;

        struct _stat fileinfo;
        _wstat(szFileName.c_str(), &fileinfo);
        size_t size = fileinfo.st_size;

        if (size > 0)
        {
            buffer.resize(size);
            size_t wchars_read = std::fread(&(buffer.front()), sizeof(wchar_t), size, fs);
            buffer.resize(wchars_read);
            buffer.shrink_to_fit();
        }

        fclose(fs);

        return buffer;
    }

    bool CConfiguration::LoadConfig(std::wstring &szFileName, CConfigList &cl)
    {
        try
        {
            std::wstring data = ReadAllText(szFileName);
            if (data.empty())
                return false;

            std::wistringstream stream;
            stream.str(data);
            for (std::wstring szBuffer; std::getline(stream, szBuffer);) 
            {
                auto parts = util::StringHelper::Split(szBuffer.c_str(), Separator);
                if (parts.size() == 2)
                {
                    CConfigEntry ce;
                    ce.szKey = parts[0];
                    ce.szValue = parts[1];
                    cl.Insert(ce);
                }
            }

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool CConfiguration::SaveConfig(std::wstring &szFileName, CConfigList &cl)
    {
        int nSize = cl.Count();
        try
        {
            FILE *fs;
            errno_t error = _wfopen_s(&fs, szFileName.c_str(), szWriteMode.c_str());
            if (error != 0)
                return false;

            std::wstring szBuffer;
            for (int i = 0; i < nSize; i++)
            {
                auto& ce = cl.Get(i);
                szBuffer = ce.szKey + szSeparator + ce.szValue + szNewChar;
                std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);
            }

            fclose(fs);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool CConfiguration::LoadFiles(std::wstring &szFileName, util::CListT<std::wstring>& fl)
    {
        try
        {
            std::wstring data = ReadAllText(szFileName);
            if (data.empty())
                return false;

            std::wistringstream stream;
            stream.str(data);
            for (std::wstring szBuffer; std::getline(stream, szBuffer);) 
            {
                if (szBuffer.size() > 0)
                {
                    util::StringHelper::Trim(szBuffer, '"');
                    if (CEncoderDefaults::IsSupportedInputExt(util::Utilities::GetFileExtension(szBuffer)) == true)
                    {
                        std::wstring szPath = szBuffer;
                        fl.Insert(szPath);
                    }
                }
                szBuffer = L"";
            }

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool CConfiguration::SaveFiles(std::wstring &szFileName, util::CListT<std::wstring>& fl, int nFormat)
    {
        int nItems = fl.Count();
        try
        {
            FILE *fs;
            errno_t error = _wfopen_s(&fs, szFileName.c_str(), szWriteMode.c_str());
            if (error != 0)
                return false;

            std::wstring szBuffer;

            for (int i = 0; i < nItems; i++)
            {
                std::wstring &szPath = fl.Get(i);
                szBuffer = (nFormat == 0 ? L"" : L"\"") + szPath + (nFormat == 0 ? L"" : L"\"") + szNewChar;
                std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);
            }

            fclose(fs);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    void CConfiguration::SearchFolderForLang(std::wstring szPath, const bool bRecurse, CLangList& m_LangLst)
    {
        try
        {
            std::vector<std::wstring> files;
            bool bResult = util::Utilities::FindFiles(szPath, files, false);
            if (bResult == true)
            {
                for (auto& file : files)
                {
                    std::wstring ext = util::Utilities::GetFileExtension(file);
                    if (util::StringHelper::TowLower(ext) == L"txt")
                    {
                        CLang lang;
                        if (this->LoadLang(file, lang.lm) == true)
                        {
                            lang.szFileName = file;
                            lang.szEnglishName = lang.lm.Get(0x00000001);
                            lang.szTargetName = lang.lm.Get(0x00000002);
                            m_LangLst.Insert(std::move(lang));
                        }
                    }
                }
            }
        }
        catch (...)
        {
            MessageBox(nullptr,
                HaveLangStrings() ? GetLangString(0x0020702A).c_str() : _T("Error while searching for files!"),
                HaveLangStrings() ? GetLangString(0x00207010).c_str() : _T("Error"),
                MB_OK | MB_ICONERROR);
        }
    }

    bool CConfiguration::LoadLang(std::wstring &szFileName, CLangMap& lm)
    {
        try
        {
            std::wstring data = ReadAllText(szFileName);
            if (data.empty())
                return false;

            lm.RemoveAll();

            std::wstring szBuffer = L"";
            std::wstring szKey = L"";
            std::wstring szValue = L"";
            int key;

            std::wistringstream stream;
            stream.str(data);

            for (std::wstring szBuffer; std::getline(stream, szBuffer);) 
            {
                auto parts = util::StringHelper::Split(szBuffer.c_str(), Separator);
                if (parts.size() == 2)
                {
                    CConfigEntry ce;
                    szKey = parts[0];
                    szValue = parts[1];
                    util::StringHelper::Replace(szValue, szNewCharVar, szNewChar);
                    util::StringHelper::Replace(szValue, szTabCharVar, szTabChar);
                    key = util::StringHelper::ToIntFromHex(szKey);
                    lm.Set(key, szValue);
                }
                szBuffer = L"";
            }

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool CConfiguration::LoadLangConfig(std::wstring &szFileName)
    {
        try
        {
            std::wstring data = ReadAllText(szFileName);
            if (data.empty())
                return false;

            std::wistringstream stream;
            stream.str(data);
            for (std::wstring szBuffer; std::getline(stream, szBuffer);) 
            {
                m_szLangFileName = szBuffer;
                return true;
            }

            return false;
        }
        catch (...)
        {
            return false;
        }
    }

    bool CConfiguration::SaveLangConfig(std::wstring &szFileName)
    {
        try
        {
            FILE *fs;
            errno_t error = _wfopen_s(&fs, szFileName.c_str(), szWriteMode.c_str());
            if (error != 0)
                return false;

            std::wstring szBuffer = m_szLangFileName + szNewChar;
            std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);

            fclose(fs);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    void CConfiguration::LoadLangStrings()
    {
        std::wstring szLangPath;
        if (m_bIsPortable == true)
        {
            szLangPath = util::Utilities::GetExeFilePath() + L"lang";
        }
        else
        {
            szLangPath = util::Utilities::GetSettingsFilePath(L"", std::wstring(DIRECTORY_CONFIG) + L"\\lang");
        }

        SearchFolderForLang(szLangPath, false, m_LangLst);

        if (m_LangLst.Count() > 0)
        {
            bool haveLang = false;
            for (int i = 0; i < m_LangLst.Count(); i++)
            {
                auto& lang = m_LangLst.Get(i);
                std::wstring szNameLang = util::Utilities::GetFileName(lang.szFileName);
                std::wstring szNameConfig = util::Utilities::GetFileName(m_szLangFileName);
                if (szNameLang == szNameConfig)
                {
                    m_nLangId = i;
                    m_bHaveLang = TRUE;
                    m_Lang = &lang.lm;
                    haveLang = true;
                    break;
                }
            }

            if (haveLang == false)
            {
                auto& lang = m_LangLst.Get(0);
                m_nLangId = 0;
                m_bHaveLang = TRUE;
                m_Lang = &lang.lm;
                m_szLangFileName = lang.szFileName;
            }
        }
        else
        {
            m_nLangId = -1;
            m_bHaveLang = FALSE;
        }
    }

    BOOL CConfiguration::HaveLangStrings()
    {
        return m_bHaveLang;
    }

    std::wstring CConfiguration::GetLangString(int id)
    {
        std::wstring szValue = L"";
        if (m_Lang != nullptr)
        {
            m_Lang->TryGet(id, szValue);
        }
        return szValue;
    }

    int CEncoderDefaults::nValidCbrBitrates[nNumValidCbrBitrates] {
        0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640
    };

    CChannelConfig CEncoderDefaults::ccAften[nNumChannelConfigAften] = {
        { 0, 0, L"1+1" },
        { 1, 0, L"1/0" },
        { 2, 0, L"2/0" },
        { 3, 0, L"3/0" },
        { 4, 0, L"2/1" },
        { 5, 0, L"3/1" },
        { 6, 0, L"2/2" },
        { 7, 0, L"3/2" },
        { 0, 1, L"1+1" },
        { 1, 1, L"1/0" },
        { 2, 1, L"2/0" },
        { 3, 1, L"3/0" },
        { 4, 1, L"2/1" },
        { 5, 1, L"3/1" },
        { 6, 1, L"2/2" },
        { 7, 1, L"3/2" }
    };

    std::wstring CEncoderDefaults::szRawSampleFormats[nNumRawSampleFormats] = {
        DEFAULT_TEXT_IGNORED,
        L"u8",
        L"s8",
        L"s16_le",
        L"s16_be",
        L"s20_le",
        L"s20_be",
        L"s24_le",
        L"s24_be",
        L"s32_le",
        L"s32_be",
        L"float_le",
        L"float_be",
        L"double_le",
        L"double_be"
    };

    std::wstring CEncoderDefaults::pszGroups[nNumEncoderOptionsGroups] = {
        L"Encoding options",
        L"Bitstream info metadata",
        L"Dynamic range compression and dialog normalization",
        L"Input options",
        L"Input filters",
        L"Alternate bit stream syntax"
    };

    std::wstring CEncoderDefaults::szCbrOption = L"-b";

    std::wstring CEncoderDefaults::szVbrOption = L"-q";

    std::wstring CEncoderDefaults::szThreadsOption = L"-threads";

    std::wstring CEncoderDefaults::szSimdOption = L"-nosimd";

    std::wstring CEncoderDefaults::szRawSampleFormatOption = L"-raw_fmt";

    std::wstring CEncoderDefaults::szRawSampleRateOption = L"-raw_sr";

    std::wstring CEncoderDefaults::szRawChannelsOption = L"-raw_ch";

    std::wstring CEncoderDefaults::szSupportedInputExt[nNumSupportedInputExt] = {
        L"wav",
        L"pcm",
        L"raw",
        L"bin",
        L"aiff",
        L"aif",
        L"aifc",
        L"avs"
    };

    int CEncoderDefaults::nSupportedInputFormats[nNumSupportedInputExt] = {
        PCM_FORMAT_WAVE,
        PCM_FORMAT_RAW,
        PCM_FORMAT_RAW,
        PCM_FORMAT_RAW,
        PCM_FORMAT_AIFF,
        PCM_FORMAT_AIFF,
        PCM_FORMAT_CAFF,
    };

    std::wstring CEncoderDefaults::szSupportedOutputExt[nNumSupportedOutputExt] = {
       L"ac3"
    };

    CEncoderOptions CEncoderDefaults::encOpt[CEncoderPreset::nNumEncoderOptions];

    void CEncoderDefaults::InitEncoderOptions()
    {
    #define SetEncoderOption(name, option, tooltip, defval, ignval, group, begin) \
        nCurOpt++; \
        encOpt[nCurOpt].szName = name; \
        encOpt[nCurOpt].szOption = option; \
        encOpt[nCurOpt].szHelpText = tooltip; \
        encOpt[nCurOpt].nDefaultValue = defval; \
        encOpt[nCurOpt].nIgnoreValue = ignval; \
        encOpt[nCurOpt].szGroupName = group; \
        encOpt[nCurOpt].bBeginGroup = begin;

    #define AddEncoderOptionValue(name, value) \
        szName = name; \
        encOpt[nCurOpt].listOptNames.Insert(szName); \
        nValue = value; \
        encOpt[nCurOpt].listOptValues.Insert(nValue);

        int nCurOpt = -1;
        std::wstring szName = _T("");
        int nValue = -1;

        ResetEncoderOptionsLists();

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00301001) : _T("Fast bit allocation"),
            _T("-fba"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00301002) :
            _T("Fast bit allocation is a less-accurate search method\n")
            _T("for CBR bit allocation. It only narrows down the SNR\n")
            _T("value to within 16 of the optimal value. The result\n")
            _T("is lower overall quality, but faster encoding. This\n")
            _T("may not give the same results each time when using\n")
            _T("parallel encoding."),
            0,
            -1,
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00208001) : _T("Encoding options"),
            true);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00301003) : _T("More accurate encoding (default)"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00301004) : _T("Faster encoding"), 1);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00302001) : _T("Exponent strategy search size"),
            _T("-exps"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00302002) :
            _T("The encoder determines the best combination of\n")
            _T("exponent strategies for a frame by searching through\n")
            _T("a list of pre-defined exponent strategies. This option\n")
            _T("controls the size of the list to be searched. The\n")
            _T("value can range from 1 (lower quality but faster) to\n")
            _T("32 (higher quality but slower). The default value is 8."),
            7,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00302003) : _T("1 (lower quality but faster)"), 1);

        for (int i = 2; i <= 7; i++)
        {
            AddEncoderOptionValue(std::to_wstring(i), i);
        }

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00302004) : _T("8 (default)"), 8);

        for (int i = 9; i <= 31; i++)
        {
            AddEncoderOptionValue(std::to_wstring(i), i);
        }

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00302005) : _T("32 (higher quality but slower)"), 32);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00303001) : _T("Start-of-stream padding"),
            _T("-pad"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00303002) :
            _T("The AC-3 format uses an overlap/add cycle for encoding\n")
            _T("each block. By default, Aften pads the delay buffer\n")
            _T("with a block of silence to avoid inaccurate encoding\n")
            _T("of the first frame of audio. If this behavior is not\n")
            _T("wanted, it can be disabled. The pad value can be a\n")
            _T("1 (default) to use padding or 0 to not use padding."),
            1,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00303003) : _T("No padding"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00303004) : _T("256 samples of padding (default)"), 1);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00304001) : _T("Bandwidth"),
            _T("-w"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00304002) :
            _T("The bandwidth setting corresponds to the high-frequency\n")
            _T("cutoff. Specifically, it sets the highest frequency bin\n")
            _T("which is encoded. The AC-3 format uses a 512-point MDCT\n")
            _T("which gives 256 frequency levels from 0 to 1/2 of the\n")
            _T("samplerate. The formula to give the number of coded\n")
            _T("frequency bins from bandwidth setting is:\n")
            _T("(w * 3) + 73, which gives a range of 73 to 253\n")
            _T("There are 2 special values, -1 and -2.\n")
            _T("When -1 is used, Aften automatically selects what\n")
            _T("it thinks is an appropriate bandwidth. This is the\n")
            _T("default setting.\n")
            _T("When -2 is used, a bandwidth is chosen for each frame\n")
            _T("based on CBR frame size and a target quality of 240.\n")
            _T("Variable bandwidth cannot be used with VBR mode."),
            0,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00304003) : _T("Fixed adaptive bandwidth (default)"), -1);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00304004) : _T("Variable adaptive bandwidth"), -2);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00304005) : _T("0 (28% of full bandwidth)"), 0);

        for (int i = 1; i <= 59; i++)
        {
            AddEncoderOptionValue(std::to_wstring(i), i);
        }

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00304006) : _T("60 (99% of full bandwidth)"), 60);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00305001) : _T("Minimum bandwidth"),
            _T("-wmin"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00305002) :
            _T("For variable bandwidth mode (-2), this option sets the\n")
            _T("minimum value for the bandwidth code. This allows the\n")
            _T("user to avoid a harsh cutoff frequency by sacrificing\n")
            _T("general audio quality. The default value is 0."),
            0,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00305003) : _T("0 (default)"), 0);

        for (int i = 1; i <= 59; i++)
        {
            AddEncoderOptionValue(std::to_wstring(i), i);
        }

        AddEncoderOptionValue(_T("60"), 60);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00306001) : _T("Maximum bandwidth"),
            _T("-wmax"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00306002) :
            _T("For variable bandwidth mode (-2), this option sets the\n")
            _T("maximum value for the bandwidth code. This can be used\n")
            _T("to speed up encoding by using a lower value than 60,\n")
            _T("which is the default."),
            60,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(_T("0"), 0);

        for (int i = 1; i <= 59; i++)
        {
            AddEncoderOptionValue(std::to_wstring(i), i);
        }

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00306003) : _T("60 (default)"), 60);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00307001) : _T("Stereo rematrixing"),
            _T("-m"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00307002) :
            _T("Using stereo rematrixing can increase quality by\n")
            _T("removing redundant information between the left and\n")
            _T("right channels. This technique is common in audio\n")
            _T("encoding, and is sometimes called mid/side encoding.\n")
            _T("When this setting is turned on, Aften adaptively turns\n")
            _T("rematrixing on or off for each of 4 frequency bands for\n")
            _T("each block. When this setting is turned off,\n")
            _T("rematrixing is not used for any blocks. The default\n")
            _T("value is 1."),
            1,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00307003) : _T("Independent L+R channels"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00307004) : _T("Mid/side rematrixing (default)"), 1);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00308001) : _T("Block switching"),
            _T("-s"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00308002) :
            _T("The AC-3 format allows for 2 different types of MDCT\n")
            _T("transformations to translate from time-domain to\n")
            _T("frequency-domain. The default is a 512-point transform,\n")
            _T("which gives better frequency resolution. There is also\n")
            _T("a 256-point transform, which gives better time\n")
            _T("resolution. The specification gives a suggested method\n")
            _T("for determining when to use the 256-point transform.\n")
            _T("When block switching is turned on, Aften uses the spec\n")
            _T("method for selecting the 256-point MDCT. When it is\n")
            _T("turned off, only the 512-point MDCT is used, which is\n")
            _T("faster. Block switching is turned off by default."),
            0,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00308003) : _T("Use only 512-point MDCT (default)"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00308004) : _T("Selectively use 256-point MDCT"), 1);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00401001) : _T("Center mix level"),
            _T("-cmix"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00401002) :
            _T("When three front channels are in use, this code\n")
            _T("indicates the nominal down mix level of the center\n")
            _T("channel with respect to the left and right channels.\n")
            _T("0 = -3.0 dB (default)\n")
            _T("1 = -4.5 dB\n")
            _T("2 = -6.0 dB"),
            0,
            -1,
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00208002) : _T("Bitstream info metadata"),
            true);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00401003) : _T("-3.0 dB (default)"), 0);
        AddEncoderOptionValue(_T("-4.5 dB"), 1);
        AddEncoderOptionValue(_T("-6.0 dB"), 2);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00402001) : _T("Surround mix level"),
            _T("-smix"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00402002) :
            _T("If surround channels are in use, this code indicates\n")
            _T("the nominal down mix level of the surround channels.\n")
            _T("0 = -3 dB (default)\n")
            _T("1 = -6 dB\n")
            _T("2 = 0"),
            0,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00402003) : _T("-3 dB (default)"), 0);
        AddEncoderOptionValue(_T("-6 dB"), 1);
        AddEncoderOptionValue(_T("0"), 2);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00403001) : _T("Dolby Surround mode"),
            _T("-dsur"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00403002) :
            _T("When operating in the two channel mode, this code\n")
            _T("indicates whether or not the program has been encoded in\n")
            _T("Dolby Surround. This information is not used by the\n")
            _T("AC-3 decoder, but may be used by other portions of the\n")
            _T("audio reproduction equipment.\n")
            _T("0 = not indicated (default)\n")
            _T("1 = not Dolby surround encoded\n")
            _T("2 = Dolby surround encoded"),
            0,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00403003) : _T("Not indicated (default)"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00403004) : _T("Not Dolby surround encoded"), 1);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00403005) : _T("Dolby surround encoded"), 2);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00501001) : _T("Dialog normalization"),
            _T("-dnorm"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00501002) :
            _T("The dialog normalization value sets the average dialog\n")
            _T("level. The value is typically constant for a particular\n")
            _T("audio program. The decoder has a target output dialog\n")
            _T("level of -31dB, so if the dialog level is specified as\n")
            _T("being -31dB already (default), the output volume is not\n")
            _T("altered. Otherwise, the overall output volume is\n")
            _T("decreased so that the dialog level is adjusted down to\n")
            _T("-31dB."),
            31,
            -1,
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00208003) : _T("Dynamic range compression and dialog normalization"),
            true);

        for (int i = 0; i <= 30; i++)
        {
            AddEncoderOptionValue(std::to_wstring(i), i);
        }

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00501003) : _T("31 (default)"), 31);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00502001) : _T("Dynamic Range Compression profile"),
            _T("-dynrng"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00502002) :
            _T("Dynamic Range Compression allows for the final output\n")
            _T("dynamic range to be limited without sacrificing quality.\n")
            _T("The full dynamic range audio is still encoded, but a\n")
            _T("code is given for each block which tells the decoder to\n")
            _T("adjust the output volume for that block. The encoder\n")
            _T("must analyze the input audio to determine the best way\n")
            _T("to compress the dynamic range based on the loudness and\n")
            _T("type of input (film, music, speech).\n")
            _T("0 = Film Light\n")
            _T("1 = Film Standard\n")
            _T("2 = Music Light\n")
            _T("3 = Music Standard\n")
            _T("4 = Speech\n")
            _T("5 = None (default)"),
            5,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00502003) : _T("Film Light"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00502004) : _T("Film Standard"), 1);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00502005) : _T("Music Light"), 2);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00502006) : _T("Music Standard"), 3);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00502007) : _T("Speech"), 4);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00502008) : _T("None (default)"), 5);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00601001) : _T("Audio coding mode (overrides wav header)"),
            _T("-acmod"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00601002) :
            _T("The acmod and lfe options allow the user to explicitly select the\n")
            _T("desired channel layout. This only controls the interpretation\n")
            _T("of the input, so no downmixing or upmixing is done.\n")
            _T("0 = 1+1 (Ch1,Ch2)\n")
            _T("1 = 1/0 (C)\n")
            _T("2 = 2/0 (L,R)\n")
            _T("3 = 3/0 (L,R,C)\n")
            _T("4 = 2/1 (L,R,S)\n")
            _T("5 = 3/1 (L,R,C,S)\n")
            _T("6 = 2/2 (L,R,SL,SR)\n")
            _T("7 = 3/2 (L,R,C,SL,SR)"),
            8,
            8,
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00208004) : _T("Input options"),
            true);

        AddEncoderOptionValue(_T("1+1 (Ch1,Ch2)"), 0);
        AddEncoderOptionValue(_T("1/0 (C)"), 1);
        AddEncoderOptionValue(_T("2/0 (L,R)"), 2);
        AddEncoderOptionValue(_T("3/0 (L,R,C)"), 3);
        AddEncoderOptionValue(_T("2/1 (L,R,S)"), 4);
        AddEncoderOptionValue(_T("3/1 (L,R,C,S)"), 5);
        AddEncoderOptionValue(_T("2/2 (L,R,SL,SR)"), 6);
        AddEncoderOptionValue(_T("3/2 (L,R,C,SL,SR)"), 7);
        AddEncoderOptionValue(DEFAULT_TEXT_IGNORED, 0);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00602001) : _T("Specify use of LFE channel (overrides wav header)"),
            _T("-lfe"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00602002) :
            _T("The acmod and lfe options allow the user to explicitly select the\n")
            _T("desired channel layout. This only controls the interpretation\n")
            _T("of the input, so no downmixing or upmixing is done.\n")
            _T("0 = LFE channel is not present\n")
            _T("1 = LFE channel is present"),
            2,
            2,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00602003) : _T("LFE channel is not present"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00602004) : _T("LFE channel is present"), 1);
        AddEncoderOptionValue(DEFAULT_TEXT_IGNORED, 0);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00603001) : _T("Specify channel configuration (overrides wav header)"),
            _T("-chconfig"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00603002) :
            _T("The chconfig option allow the user to explicitly select the\n")
            _T("desired channel layout. This only controls the interpretation\n")
            _T("of the input, so no downmixing or upmixing is done.\n")
            _T("1+1 = (Ch1,Ch2)\n")
            _T("1/0 = (C)\n")
            _T("2/0 = (L,R)\n")
            _T("3/0 = (L,R,C)\n")
            _T("2/1 = (L,R,S)\n")
            _T("3/1 = (L,R,C,S)\n")
            _T("2/2 = (L,R,SL,SR)\n")
            _T("3/2 = (L,R,C,SL,SR)\n")
            _T("Adding +LFE indicates use of the LFE channel"),
            16,
            16,
            _T(""),
            false);

        AddEncoderOptionValue(_T("1+1 = (Ch1,Ch2)"), 0);
        AddEncoderOptionValue(_T("1/0 = (C)"), 1);
        AddEncoderOptionValue(_T("2/0 = (L,R)"), 2);
        AddEncoderOptionValue(_T("3/0 = (L,R,C)"), 3);
        AddEncoderOptionValue(_T("2/1 = (L,R,S)"), 4);
        AddEncoderOptionValue(_T("3/1 = (L,R,C,S)"), 5);
        AddEncoderOptionValue(_T("2/2 = (L,R,SL,SR)"), 6);
        AddEncoderOptionValue(_T("3/2 = (L,R,C,SL,SR)"), 7);
        AddEncoderOptionValue(_T("1+1+LFE = (Ch1,Ch2) + LFE"), 8);
        AddEncoderOptionValue(_T("1/0+LFE = (C) + LFE"), 9);
        AddEncoderOptionValue(_T("2/0+LFE = (L,R) + LFE"), 10);
        AddEncoderOptionValue(_T("3/0+LFE = (L,R,C) + LFE"), 11);
        AddEncoderOptionValue(_T("2/1+LFE = (L,R,S) + LFE"), 12);
        AddEncoderOptionValue(_T("3/1+LFE = (L,R,C,S) + LFE"), 13);
        AddEncoderOptionValue(_T("2/2+LFE = (L,R,SL,SR) + LFE"), 14);
        AddEncoderOptionValue(_T("3/2+LFE = (L,R,C,SL,SR) + LFE"), 15);
        AddEncoderOptionValue(DEFAULT_TEXT_IGNORED, 0);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00604001) : _T("Channel mapping order of input audio"),
            _T("-chmap"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00604002) :
            _T("Some programs create WAVE files which use a channel\n")
            _T("mapping other than the standard WAVE mapping. This\n")
            _T("option allows the user to specify if the input file\n")
            _T("uses WAVE, AC-3, or MPEG channel mapping. The MPEG\n")
            _T("channel mapping is used by DTS and by MPEG-2/4 formats\n")
            _T("such as MP2 and AAC.\n")
            _T("0 = WAVE mapping (default)\n")
            _T("1 = AC-3 mapping\n")
            _T("2 = MPEG mapping"),
            0,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00604003) : _T("WAV mapping (default)"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00604004) : _T("AC-3 mapping"), 1);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00604005) : _T("MPEG mapping"), 2);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00605001) : _T("Read input WAVE audio data until the end-of-file"),
            _T("-readtoeof"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00605002) :
            _T("This overrides the data size in the WAVE header, and\n")
            _T("can be useful for streaming input or files larger than\n")
            _T("4 GB.\n")
            _T("0 = use data size in header (default)\n")
            _T("1 = read data until end-of-file"),
            0,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00605003) : _T("Use data size in header (default)"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00605004) : _T("Read data until end-of-file"), 1);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00701001) : _T("Specify use of the bandwidth low-pass filter"),
            _T("-bwfilter"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00701002) :
            _T("The bandwidth low-pass filter pre-filters the input\n")
            _T("audio before converting to frequency-domain. This\n")
            _T("smooths the cutoff frequency transition for slightly\n")
            _T("better quality.\n")
            _T("0 = do not apply filter (default)\n")
            _T("1 = apply filter"),
            0,
            -1,
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00208005) : _T("Input filters"),
            true);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00701003) : _T("Do not apply filter (default)"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00701004) : _T("Apply filter"), 1);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00702001) : _T("Specify use of the DC high-pass filter"),
            _T("-dcfilter"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00702002) :
            _T("The DC high-pass filter is listed as optional by the\n")
            _T("AC-3 specification. The implementation, as suggested,\n")
            _T("is a single pole filter at 3 Hz.\n")
            _T("0 = do not apply filter (default)\n")
            _T("1 = apply filter"),
            0,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00702003) : _T("Do not apply filter (default)"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00702004) : _T("Apply filter"), 1);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00703001) : _T("Specify use of the LFE low-pass filter"),
            _T("-lfefilter"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00703002) :
            _T("The LFE low-pass filter is recommended by the AC-3\n")
            _T("specification. The cutoff is 120 Hz. The specification\n")
            _T("recommends an 8th order elliptic filter, but instead,\n")
            _T("Aften uses a Butterworth 2nd order cascaded direct\n")
            _T("form II filter.\n")
            _T("0 = do not apply filter (default)\n")
            _T("1 = apply filter"),
            0,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00703003) : _T("Do not apply filter (default)"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00703004) : _T("Apply filter"), 1);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00801001) : _T("Specify use of extended bitstream info 1"),
            _T("-xbsi1"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00801002) :
            _T("Extended bitstream info 1 contains the dmixmod,\n")
            _T("ltrtcmix, ltrtsmix, lorocmix, and lorosmix fields. If\n")
            _T("this option is turned on, all these values are written\n")
            _T("to the output stream.\n")
            _T("0 = do not write xbsi1\n")
            _T("1 = write xbsi1"),
            2,
            2,
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00208006) : _T("Alternate bit stream syntax"),
            true);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00801003) : _T("Do not write xbsi1"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00801004) : _T("Write xbsi1"), 1);
        AddEncoderOptionValue(DEFAULT_TEXT_IGNORED, 0);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00802001) : _T("Preferred stereo downmix mode"),
            _T("-dmixmod"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00802002) :
            _T("This code indicates the type of stereo downmix preferred\n")
            _T("by the mastering engineer, and can be optionally used,\n")
            _T("overridden, or ignored by the decoder.\n")
            _T("0 = not indicated (default)\n")
            _T("1 = Lt/Rt downmix preferred\n")
            _T("2 = Lo/Ro downmix preferred"),
            0,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00802003) : _T("Not indicated (default)"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00802004) : _T("Lt/Rt downmix preferred"), 1);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00802005) : _T("Lo/Ro downmix preferred"), 2);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00803001) : _T("Lt/Rt center mix level"),
            _T("-ltrtcmix"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00803002) :
            _T("This code indicates the nominal down mix level of the\n")
            _T("center channel with respect to the left and right\n")
            _T("channels in an Lt/Rt downmix.\n")
            _T("0 = +3.0 dB\n")
            _T("1 = +1.5 dB\n")
            _T("2 =  0.0 dB\n")
            _T("3 = -1.5 dB\n")
            _T("4 = -3.0 dB (default)\n")
            _T("5 = -4.5 dB\n")
            _T("6 = -6.0 dB\n")
            _T("7 = -inf dB"),
            4,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(_T("+3.0 dB"), 0);
        AddEncoderOptionValue(_T("+1.5 dB"), 1);
        AddEncoderOptionValue(_T("0.0 dB"), 2);
        AddEncoderOptionValue(_T("-1.5 dB"), 3);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00803003) : _T("-3.0 dB (default)"), 4);
        AddEncoderOptionValue(_T("-4.5 dB"), 5);
        AddEncoderOptionValue(_T("-6.0 dB"), 6);
        AddEncoderOptionValue(_T("-inf dB"), 7);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00804001) : _T("Lt/Rt surround mix level"),
            _T("-ltrtsmix"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00804002) :
            _T("This code indicates the nominal down mix level of the\n")
            _T("surround channels with respect to the left and right\n")
            _T("channels in an Lt/Rt downmix.\n")
            _T("0 = +3.0 dB\n")
            _T("1 = +1.5 dB\n")
            _T("2 =  0.0 dB\n")
            _T("3 = -1.5 dB\n")
            _T("4 = -3.0 dB (default)\n")
            _T("5 = -4.5 dB\n")
            _T("6 = -6.0 dB\n")
            _T("7 = -inf dB"),
            4,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(_T("+3.0 dB"), 0);
        AddEncoderOptionValue(_T("+1.5 dB"), 1);
        AddEncoderOptionValue(_T("0.0 dB"), 2);
        AddEncoderOptionValue(_T("-1.5 dB"), 3);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00804003) : _T("-3.0 dB (default)"), 4);
        AddEncoderOptionValue(_T("-4.5 dB"), 5);
        AddEncoderOptionValue(_T("-6.0 dB"), 6);
        AddEncoderOptionValue(_T("-inf dB"), 7);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00805001) : _T("Lo/Ro center mix level"),
            _T("-lorocmix"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00805002) :
            _T("This code indicates the nominal down mix level of the\n")
            _T("center channel with respect to the left and right\n")
            _T("channels in an Lo/Ro downmix.\n")
            _T("0 = +3.0 dB\n")
            _T("1 = +1.5 dB\n")
            _T("2 =  0.0 dB\n")
            _T("3 = -1.5 dB\n")
            _T("4 = -3.0 dB (default)\n")
            _T("5 = -4.5 dB\n")
            _T("6 = -6.0 dB\n")
            _T("7 = -inf dB"),
            4,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(_T("+3.0 dB"), 0);
        AddEncoderOptionValue(_T("+1.5 dB"), 1);
        AddEncoderOptionValue(_T("0.0 dB"), 2);
        AddEncoderOptionValue(_T("-1.5 dB"), 3);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00805003) : _T("-3.0 dB (default)"), 4);
        AddEncoderOptionValue(_T("-4.5 dB"), 5);
        AddEncoderOptionValue(_T("-6.0 dB"), 6);
        AddEncoderOptionValue(_T("-inf dB"), 7);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00806001) : _T("Lo/Ro surround mix level"),
            _T("-lorosmix"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00806002) :
            _T("This code indicates the nominal down mix level of the\n")
            _T("surround channels with respect to the left and right\n")
            _T("channels in an Lo/Ro downmix.\n")
            _T("0 = +3.0 dB\n")
            _T("1 = +1.5 dB\n")
            _T("2 =  0.0 dB\n")
            _T("3 = -1.5 dB\n")
            _T("4 = -3.0 dB (default)\n")
            _T("5 = -4.5 dB\n")
            _T("6 = -6.0 dB\n")
            _T("7 = -inf dB"),
            4,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(_T("+3.0 dB"), 0);
        AddEncoderOptionValue(_T("+1.5 dB"), 1);
        AddEncoderOptionValue(_T("0.0 dB"), 2);
        AddEncoderOptionValue(_T("-1.5 dB"), 3);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00806003) : _T("-3.0 dB (default)"), 4);
        AddEncoderOptionValue(_T("-4.5 dB"), 5);
        AddEncoderOptionValue(_T("-6.0 dB"), 6);
        AddEncoderOptionValue(_T("-inf dB"), 7);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00807001) : _T("Specify use of extended bitstream info 2"),
            _T("-xbsi2"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00807002) :
            _T("Extended bitstream info 2 contains the dsurexmod,\n")
            _T("dheadphon, and adconvtyp fields. If this option is\n")
            _T("turned on, all these values are written to the output\n")
            _T("stream. These options are not used by the AC-3 decoder,\n")
            _T("but may be used by other portions of the audio\n")
            _T("reproduction equipment.\n")
            _T("0 = do not write xbsi2\n")
            _T("1 = write xbsi2"),
            2,
            2,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00807003) : _T("Do not write xbsi2"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00807004) : _T("Write xbsi2"), 1);
        AddEncoderOptionValue(DEFAULT_TEXT_IGNORED, 0);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00808001) : _T("Dolby Surround EX mode"),
            _T("-dsurexmod"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00808002) :
            _T("This code indicates whether or not the program has been\n")
            _T("encoded in Dolby Surround EX.\n")
            _T("0 = not indicated (default)\n")
            _T("1 = Not Dolby Surround EX encoded\n")
            _T("2 = Dolby Surround EX encoded"),
            0,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00808003) : _T("Not indicated (default)"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00808004) : _T("Not Dolby Surround EX encoded"), 1);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00808005) : _T("Dolby Surround EX encoded"), 2);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00809001) : _T("Dolby Headphone mode"),
            _T("-dheadphon"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00809002) :
            _T("This code indicates whether or not the program has been\n")
            _T("Dolby Headphone-encoded.\n")
            _T("0 = not indicated (default)\n")
            _T("1 = Not Dolby Headphone encoded\n")
            _T("2 = Dolby Headphone encoded"),
            0,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00809003) : _T("Not indicated (default)"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00809004) : _T("Not Dolby Headphone encoded"), 1);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00809005) : _T("Dolby Headphone encoded"), 2);

        SetEncoderOption(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x0080A001) : _T("A/D converter type"),
            _T("-adconvtyp"),
            config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x0080A002) :
            _T("This code indicates the type of A/D converter technology\n")
            _T("used to capture the PCM audio.\n")
            _T("0 = Standard (default)\n")
            _T("1 = HDCD"),
            0,
            -1,
            _T(""),
            false);

        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x0080A003) : _T("Standard (default)"), 0);
        AddEncoderOptionValue(config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x0080A004) : _T("HDCD"), 1);
    }

    int CEncoderDefaults::FindValidBitratePos(const int nBitrate)
    {
        for (int i = 0; i < nNumValidCbrBitrates; i++)
        {
            if (nValidCbrBitrates[i] == nBitrate)
                return i;
        }
        return 0;
    }

    int CEncoderDefaults::FindOptionIndex(std::wstring szOption)
    {
        for (int i = 0; i < CEncoderPreset::nNumEncoderOptions; i++)
        {
            std::wstring szBuffer = encOpt[i].szOption;
            util::StringHelper::TrimLeft(szBuffer, '-');
            if (szOption == szBuffer)
            {
                return i;
            }
        }
        return 0;
    }

    void CEncoderDefaults::ResetEncoderOptionsLists()
    {
        for (int i = 0; i < CEncoderPreset::nNumEncoderOptions; i++)
        {
            if (encOpt[i].listOptNames.Count() > 0)
                encOpt[i].listOptNames.RemoveAll();

            if (encOpt[i].listOptValues.Count() > 0)
                encOpt[i].listOptValues.RemoveAll();
        }
    }

    void CEncoderDefaults::ParseEncoderPreset(CEncoderPreset &preset, CConfigList &cl)
    {
        for (int i = 0; i < cl.Count(); i++)
        {
            std::wstring szBuffer;
            auto& ce = cl.Get(i);

            if (ce.szKey == L"engine")
            {
                preset.nCurrentEngine = util::StringHelper::ToInt(ce.szValue);
                continue;
            }

            szBuffer = szThreadsOption;
            util::StringHelper::TrimLeft(szBuffer, '-');
            if (ce.szKey == szBuffer)
            {
                preset.nThreads = util::StringHelper::ToInt(ce.szValue);
                continue;
            }

            if (ce.szKey == L"mmx")
            {
                preset.nUsedSIMD[0] = util::StringHelper::ToInt(ce.szValue);
                continue;
            }

            if (ce.szKey == L"sse")
            {
                preset.nUsedSIMD[1] = util::StringHelper::ToInt(ce.szValue);
                continue;
            }

            if (ce.szKey == L"sse2")
            {
                preset.nUsedSIMD[2] = util::StringHelper::ToInt(ce.szValue);
                continue;
            }

            if (ce.szKey == L"sse3")
            {
                preset.nUsedSIMD[3] = util::StringHelper::ToInt(ce.szValue);
                continue;
            }

            if (ce.szKey == L"mode")
            {
                preset.nMode = (AftenEncMode)util::StringHelper::ToInt(ce.szValue);
                continue;
            }

            szBuffer = szCbrOption;
            util::StringHelper::TrimLeft(szBuffer, '-');
            if (ce.szKey == szBuffer)
            {
                preset.nBitrate = util::StringHelper::ToInt(ce.szValue);
                continue;
            }

            szBuffer = szVbrOption;
            util::StringHelper::TrimLeft(szBuffer, '-');
            if (ce.szKey == szBuffer)
            {
                preset.nQuality = util::StringHelper::ToInt(ce.szValue);
                continue;
            }

            szBuffer = szRawSampleFormatOption;
            util::StringHelper::TrimLeft(szBuffer, '-');
            if (ce.szKey == szBuffer)
            {
                preset.nRawSampleFormat = util::StringHelper::ToInt(ce.szValue);
                continue;
            }

            szBuffer = szRawSampleRateOption;
            util::StringHelper::TrimLeft(szBuffer, '-');
            if (ce.szKey == szBuffer)
            {
                preset.nRawSampleRate = util::StringHelper::ToInt(ce.szValue);
                continue;
            }

            szBuffer = szRawChannelsOption;
            util::StringHelper::TrimLeft(szBuffer, '-');
            if (ce.szKey == szBuffer)
            {
                preset.nRawChannels = util::StringHelper::ToInt(ce.szValue);
                continue;
            }

            for (int i = 0; i < CEncoderPreset::nNumEncoderOptions; i++)
            {
                szBuffer = encOpt[i].szOption;
                util::StringHelper::TrimLeft(szBuffer, '-');
                if (ce.szKey == szBuffer)
                {
                    preset.nSetting[i] = util::StringHelper::ToInt(ce.szValue);
                    break;
                }
            }
        }
    }

    bool CEncoderDefaults::LoadEncoderPresets(CEncoderPresetList& encPresets, std::wstring& szFileName, CEncoderPreset& defaultPreset)
    {
        try
        {
            std::wstring data = ReadAllText(szFileName);
            if (data.empty())
                return false;

            CEncoderPreset temp;
            CConfigList cl;
            bool bHavePreset = false;
            encPresets.RemoveAll();

            std::wistringstream stream;
            stream.str(data);
            for (std::wstring szBuffer; std::getline(stream, szBuffer);) 
            {
                if ((szBuffer[0] == '[') && (szBuffer[szBuffer.size() - 1] == ']'))
                {
                    if (bHavePreset == true)
                    {
                        ParseEncoderPreset(temp, cl);
                        auto preset = temp;
                        encPresets.Insert(preset);
                        cl.RemoveAll();
                    }

                    temp = defaultPreset;
                    util::StringHelper::TrimLeft(szBuffer, '[');
                    util::StringHelper::TrimRight(szBuffer, ']');
                    temp.szName = szBuffer;
                    bHavePreset = true;
                }
                else
                {
                    auto parts = util::StringHelper::Split(szBuffer.c_str(), Separator);
                    if (parts.size() == 2)
                    {
                        CConfigEntry ce;
                        ce.szKey = parts[0];
                        ce.szValue = parts[1];
                        cl.Insert(ce);
                    }
                }
            }

            if (bHavePreset == true)
            {
                ParseEncoderPreset(temp, cl);
                auto preset = temp;
                encPresets.Insert(preset);
                cl.RemoveAll();
            }

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool CEncoderDefaults::SaveEncoderPresets(CEncoderPresetList& encPresets, std::wstring& szFileName, CEncoderPreset& defaultPreset)
    {
        const int nSize = (const int)encPresets.Count();
        try
        {
            FILE *fs;
            errno_t error = _wfopen_s(&fs, szFileName.c_str(), szWriteMode.c_str());
            if (error != 0)
                return false;

            std::wstring szBuffer;
            std::wstring szTmpBuffer;

            for (int i = 0; i < nSize; i++)
            {
                auto& preset = encPresets.Get(i);

                szBuffer = L"[" + preset.szName + L"]\n";
                std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);

                szBuffer = L"engine" + szSeparator + std::to_wstring(preset.nCurrentEngine) + szNewChar;
                std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);

                szTmpBuffer = szThreadsOption;
                util::StringHelper::TrimLeft(szTmpBuffer, '-');
                szBuffer = szTmpBuffer + szSeparator + std::to_wstring(preset.nThreads) + szNewChar;
                std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);

                szBuffer = L"mmx" + szSeparator + std::to_wstring(preset.nUsedSIMD[0]) + szNewChar;
                std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);

                szBuffer = L"sse" + szSeparator + std::to_wstring(preset.nUsedSIMD[1]) + szNewChar;
                std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);

                szBuffer = L"sse2" + szSeparator + std::to_wstring(preset.nUsedSIMD[2]) + szNewChar;
                std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);

                szBuffer = L"sse3" + szSeparator + std::to_wstring(preset.nUsedSIMD[3]) + szNewChar;
                std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);

                szBuffer = L"mode" + szSeparator + std::to_wstring(preset.nMode) + szNewChar;
                std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);

                szTmpBuffer = szCbrOption;
                util::StringHelper::TrimLeft(szTmpBuffer, '-');
                szBuffer = szTmpBuffer + szSeparator + std::to_wstring(preset.nBitrate) + szNewChar;
                std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);

                szTmpBuffer = szVbrOption;
                util::StringHelper::TrimLeft(szTmpBuffer, '-');
                szBuffer = szTmpBuffer + szSeparator + std::to_wstring(preset.nQuality) + szNewChar;
                std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);

                szTmpBuffer = szRawSampleFormatOption;
                util::StringHelper::TrimLeft(szTmpBuffer, '-');
                szBuffer = szTmpBuffer + szSeparator + std::to_wstring(preset.nRawSampleFormat) + szNewChar;
                std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);

                szTmpBuffer = szRawSampleRateOption;
                util::StringHelper::TrimLeft(szTmpBuffer, '-');
                szBuffer = szTmpBuffer + szSeparator + std::to_wstring(preset.nRawSampleRate) + szNewChar;
                std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);

                szTmpBuffer = szRawChannelsOption;
                util::StringHelper::TrimLeft(szTmpBuffer, '-');
                szBuffer = szTmpBuffer + szSeparator + std::to_wstring(preset.nRawChannels) + szNewChar;
                std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);

                for (int j = 0; j < CEncoderPreset::nNumEncoderOptions; j++)
                {
                    szTmpBuffer = encOpt[j].szOption;
                    util::StringHelper::TrimLeft(szTmpBuffer, '-');
                    szBuffer = szTmpBuffer + szSeparator+ std::to_wstring(preset.nSetting[j]) + szNewChar;
                    std::fwrite(szBuffer.data(), sizeof(wchar_t), szBuffer.size(), fs);
                }
            }

            fclose(fs);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool CEncoderDefaults::IsSupportedInputExt(std::wstring &szExt)
    {
        for (int i = 0; i < nNumSupportedInputExt; i++)
        {
            if (util::StringHelper::CompareNoCase(szExt, szSupportedInputExt[i]))
                return true;
        }
        return false;
    }

    int CEncoderDefaults::GetSupportedInputFormat(std::wstring &szExt)
    {
        for (int i = 0; i < nNumSupportedInputExt; i++)
        {
            if (util::StringHelper::CompareNoCase(szExt, szSupportedInputExt[i]))
            {
                return nSupportedInputFormats[i];
            }
        }
        return PCM_FORMAT_UNKNOWN;
    }

    CAtlString CEncoderDefaults::GetSupportedInputFilesFilter()
    {
        CAtlString szFilter = _T("");
        CAtlString szExtL = _T("");
        CAtlString szExtU = _T("");
        CAtlString szBuff = _T("");

        for (int i = 0; i < nNumSupportedInputExt; i++)
        {
            szExtL = szSupportedInputExt[i].c_str();
            szBuff = _T("*.") + szExtL.MakeLower();
            szBuff += (i < nNumSupportedInputExt - 1) ? _T(";") : _T("");
            szFilter += szBuff;
        }

        szFilter = (config::m_Config.HaveLangStrings() ? CAtlString(config::m_Config.GetLangString(0x00207006).c_str()) : _T("Supported Files")) +
            _T(" (") + szFilter + _T(")|") + szFilter + _T("|");

        for (int i = 0; i < nNumSupportedInputExt; i++)
        {
            szExtL = szExtU = szSupportedInputExt[i].c_str();
            szExtU.MakeUpper();
            szExtL.MakeLower();

            szBuff.Format(_T("%s %s (*.%s)|*.%s|"),
                szExtU,
                config::m_Config.HaveLangStrings() ? config::m_Config.GetLangString(0x00207007).c_str() : _T("Files"),
                szExtL, szExtL);

            szFilter += szBuff;
        }

        szFilter += (config::m_Config.HaveLangStrings() ? CAtlString(config::m_Config.GetLangString(0x00207008).c_str()) : _T("All Files")) +
            _T(" (*.*)|*.*||");

        return szFilter;
    }
}