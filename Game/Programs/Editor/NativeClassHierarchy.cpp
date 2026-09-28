#include "NativeClassHierarchy.h"
#include <Windows.h>
#include <oleauto.h>
#include "ThirdParty/WIL/include/wil/com.h"
#include "ThirdParty/WIL/include/wil/resource.h"
#include <fstream>
#include <regex>
#include <algorithm>
#include <set>
#include <map>
#include <cstring>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

namespace
{
    std::string ReadText(const std::filesystem::path& _Path)
    {
        std::ifstream File(_Path, std::ios::binary | std::ios::ate);
        if (!File || File.tellg() < 0 || File.tellg() > 16 * 1024 * 1024) return {};
        std::string Text(static_cast<size_t>(File.tellg()), '\0');
        File.seekg(0);
        File.read(Text.data(), Text.size());
        return File ? Text : std::string();
    }

    std::string CodeOnly(std::string _Text)
    {
        for (size_t I = 0; I < _Text.size();)
        {
            size_t End = I;
            if (_Text.compare(I, 2, "//") == 0)
                End = _Text.find('\n', I + 2);
            else if (_Text.compare(I, 2, "/*") == 0)
            {
                End = _Text.find("*/", I + 2);
                if (End != std::string::npos) End += 2;
            }
            else if (_Text.compare(I, 2, "R\"") == 0)
            {
                const auto Open = _Text.find('(', I + 2);
                if (Open != std::string::npos && Open - I <= 18)
                {
                    const auto Closing = ")" + _Text.substr(I + 2, Open - I - 2) + "\"";
                    End = _Text.find(Closing, Open + 1);
                    if (End != std::string::npos) End += Closing.size();
                }
            }
            else if (_Text[I] == '"' || _Text[I] == '\'')
            {
                const char Quote = _Text[I];
                End = I + 1;
                while (End < _Text.size())
                {
                    if (_Text[End] == '\\') End = (std::min)(End + 2, _Text.size());
                    else if (_Text[End++] == Quote) break;
                }
            }
            if (End == I) { ++I; continue; }
            if (End == std::string::npos) End = _Text.size();
            while (I < End) { if (_Text[I] != '\n' && _Text[I] != '\r') _Text[I] = ' '; ++I; }
        }
        return _Text;
    }

    std::filesystem::path XmlPath(std::string _Text)
    {
        const std::pair<const char*, const char*> Entities[] = {{"&quot;", "\""}, {"&apos;", "'"}, {"&lt;", "<"}, {"&gt;", ">"}, {"&amp;", "&"}};
        for (const auto& Entity : Entities)
            for (size_t Offset = 0; (Offset = _Text.find(Entity.first, Offset)) != std::string::npos; Offset += std::strlen(Entity.second))
                _Text.replace(Offset, std::strlen(Entity.first), Entity.second);
        return std::filesystem::path(std::u8string(_Text.begin(), _Text.end())).lexically_normal();
    }

    std::map<std::filesystem::path, std::filesystem::path> ReadClassFilters(const std::filesystem::path& _Project)
    {
        std::map<std::filesystem::path, std::filesystem::path> Result;
        const auto Xml = std::regex_replace(ReadText(std::filesystem::path(_Project.wstring() + L".filters")), std::regex("<!--[\\s\\S]*?-->"), "");
        const std::regex Item(R"xml(<ClInclude\s+Include\s*=\s*"([^"]+)"\s*>\s*<Filter>\s*([^<]*?)\s*</Filter>\s*</ClInclude>)xml");
        for (std::sregex_iterator It(Xml.begin(), Xml.end(), Item), End; It != End; ++It)
        {
            const auto Header = std::filesystem::absolute(_Project.parent_path() / XmlPath((*It)[1].str())).lexically_normal();
            const auto Folder = XmlPath((*It)[2].str());
            if (Folder.is_absolute() || Folder.has_root_path()) continue;
            bool Valid = true;
            for (const auto& Part : Folder) if (Part == L"..") Valid = false;
            if (Valid) Result.insert_or_assign(Header, Folder == L"." ? std::filesystem::path() : Folder);
        }
        return Result;
    }

    struct FVariant
    {
        VARIANT __Value;
        FVariant() { VariantInit(&__Value); }
        ~FVariant() { VariantClear(&__Value); }
        FVariant(const FVariant&) = delete;
        FVariant& operator=(const FVariant&) = delete;
    };

    bool Invoke(IDispatch* _Object, const wchar_t* _Name, WORD _Flags, VARIANT* _Args, UINT _Count, FVariant& _Result)
    {
        if (!_Object) return false;
        LPOLESTR Name = const_cast<LPOLESTR>(_Name);
        DISPID ID;
        if (FAILED(_Object->GetIDsOfNames(IID_NULL, &Name, 1, LOCALE_USER_DEFAULT, &ID))) return false;
        DISPPARAMS Parameters = {_Args, nullptr, _Count, 0};
        return SUCCEEDED(_Object->Invoke(ID, IID_NULL, LOCALE_USER_DEFAULT, _Flags, &Parameters, &_Result.__Value, nullptr, nullptr));
    }

    IDispatch* Dispatch(FVariant& _Value)
    {
        return _Value.__Value.vt == VT_DISPATCH ? _Value.__Value.pdispVal : nullptr;
    }

    bool OpenInDte(IDispatch* _Dte, const std::filesystem::path& _Solution, const FNativeClassSource& _Class)
    {
        FVariant Solution, Name, Operations, Document, Selection, Result;
        if (!Invoke(_Dte, L"Solution", DISPATCH_PROPERTYGET, nullptr, 0, Solution) ||
            !Invoke(Dispatch(Solution), L"FullName", DISPATCH_PROPERTYGET, nullptr, 0, Name) ||
            Name.__Value.vt != VT_BSTR || !Name.__Value.bstrVal) return false;
        std::error_code Error;
        if (!std::filesystem::equivalent(std::filesystem::path(Name.__Value.bstrVal), _Solution, Error)) return false;
        if (!Invoke(_Dte, L"ItemOperations", DISPATCH_PROPERTYGET, nullptr, 0, Operations)) return false;
        auto ViewKind = wil::make_variant_bstr_nothrow(L"{00000000-0000-0000-0000-000000000000}");
        auto Header = wil::make_variant_bstr_nothrow(_Class.__Header.c_str());
        if (ViewKind.vt != VT_BSTR || Header.vt != VT_BSTR) return false;
        VARIANT Args[2] = {ViewKind, Header};
        const bool Opened = Invoke(Dispatch(Operations), L"OpenFile", DISPATCH_METHOD, Args, 2, Result);
        if (!Opened) return false;
        if (Invoke(_Dte, L"ActiveDocument", DISPATCH_PROPERTYGET, nullptr, 0, Document) &&
            Invoke(Dispatch(Document), L"Selection", DISPATCH_PROPERTYGET, nullptr, 0, Selection))
        {
            VARIANT Position[2] = {};
            Position[0].vt = VT_BOOL; Position[0].boolVal = VARIANT_FALSE;
            Position[1].vt = VT_I4; Position[1].lVal = _Class.__Line;
            FVariant Ignored;
            Invoke(Dispatch(Selection), L"GotoLine", DISPATCH_METHOD, Position, 2, Ignored);
        }
        FVariant Window, Activated;
        if (Invoke(_Dte, L"MainWindow", DISPATCH_PROPERTYGET, nullptr, 0, Window))
            Invoke(Dispatch(Window), L"Activate", DISPATCH_METHOD, nullptr, 0, Activated);
        return true;
    }
}

std::vector<FNativeClassSource> ReadNativeClassSources(const std::filesystem::path& _Project, const FString& _Module, bool _Engine)
{
    std::vector<FNativeClassSource> Result;
    const auto Xml = std::regex_replace(ReadText(_Project), std::regex("<!--[\\s\\S]*?-->"), "");
    const std::regex Includes("<ClInclude\\s+Include=\"([^\"]+)\"");
    const std::regex Marker("\\bUCLASS\\s*\\(");
    const std::regex Declaration("^\\s*class\\s+(?:[A-Za-z_][A-Za-z0-9_]*_API\\s+)?([A-Za-z_][A-Za-z0-9_]*)\\b[^;{]*\\{");
    std::set<std::filesystem::path> Seen;
    const auto Filters = ReadClassFilters(_Project);
    for (std::sregex_iterator It(Xml.begin(), Xml.end(), Includes), End; It != End; ++It)
    {
        auto RelativeText = (*It)[1].str();
        const std::pair<const char*, const char*> Entities[] = {{"&amp;", "&"}, {"&quot;", "\""}, {"&apos;", "'"}, {"&lt;", "<"}, {"&gt;", ">"}};
        for (const auto& Entity : Entities)
            for (size_t Offset = 0; (Offset = RelativeText.find(Entity.first, Offset)) != std::string::npos; Offset += std::strlen(Entity.second))
                RelativeText.replace(Offset, std::strlen(Entity.first), Entity.second);
        if (RelativeText.find("$(") != std::string::npos) continue;
        const auto Header = std::filesystem::absolute(_Project.parent_path() / std::filesystem::path(std::u8string(RelativeText.begin(), RelativeText.end()))).lexically_normal();
        const auto Relative = Header.lexically_relative(std::filesystem::absolute(_Project.parent_path()).lexically_normal());
        if (Relative.empty() || *Relative.begin() == L".." || !Seen.insert(Header).second) continue;
        auto Text = ReadText(Header);
        if (Text.find("UCLASS") == std::string::npos) continue;
        const auto Code = CodeOnly(std::move(Text));
        if (Code.find("UCLASS") == std::string::npos) continue;
        for (std::sregex_iterator ClassIt(Code.begin(), Code.end(), Marker); ClassIt != End; ++ClassIt)
        {
            size_t Offset = static_cast<size_t>(ClassIt->position() + ClassIt->length());
            int Depth = 1;
            while (Offset < Code.size() && Depth)
            {
                if (Code[Offset] == '(') ++Depth;
                if (Code[Offset] == ')') --Depth;
                ++Offset;
            }
            if (Depth) continue;
            std::smatch Match;
            const std::string Tail = Code.substr(Offset);
            if (!std::regex_search(Tail, Match, Declaration)) continue;
            const size_t Position = Offset + Match.position(1);
            const auto Filter = Filters.find(Header);
            Result.push_back({Match[1].str(), _Module, Header, Filter != Filters.end() ? Filter->second : Relative.parent_path(),
                1 + static_cast<int>(std::count(Code.begin(), Code.begin() + Position, '\n')), _Engine});
        }
    }
    std::sort(Result.begin(), Result.end(), [](const auto& _A, const auto& _B) { return _A.__Name < _B.__Name; });
    return Result;
}

bool OpenNativeClassSource(const std::filesystem::path& _Solution, const FNativeClassSource& _Class, FString& _Error, bool _LaunchIfMissing)
{
    std::error_code Error;
    if (!std::filesystem::is_regular_file(_Class.__Header, Error)) { _Error = "The class header no longer exists."; return false; }
    bool Opened = false;
    {
        const HRESULT Initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        wil::unique_couninitialize_call Uninitialize(SUCCEEDED(Initialized));
        wil::com_ptr_nothrow<IRunningObjectTable> Table;
        if (SUCCEEDED(GetRunningObjectTable(0, Table.put())))
        {
            wil::com_ptr_nothrow<IEnumMoniker> Items;
            wil::com_ptr_nothrow<IBindCtx> Context;
            if (SUCCEEDED(Table->EnumRunning(Items.put())) && SUCCEEDED(CreateBindCtx(0, Context.put())))
            {
                while (!Opened)
                {
                    wil::com_ptr_nothrow<IMoniker> Item;
                    if (Items->Next(1, Item.put(), nullptr) != S_OK) break;
                    wil::unique_cotaskmem_string Name;
                    if (SUCCEEDED(Item->GetDisplayName(Context.get(), nullptr, Name.put())))
                    {
                        if (FString(Name.get()).Contains(TEXT("VisualStudio.DTE.17.0"), ESearchCase::CaseSensitive))
                        {
                            wil::com_ptr_nothrow<IUnknown> Object;
                            if (SUCCEEDED(Table->GetObject(Item.get(), Object.put())))
                            {
                                wil::com_ptr_nothrow<IDispatch> Dte;
                                if (SUCCEEDED(Object->QueryInterface(IID_PPV_ARGS(Dte.put()))))
                                    Opened = OpenInDte(Dte.get(), _Solution, _Class);
                            }
                        }
                    }
                }
            }
        }
    }
    if (Opened) return true;
    if (!_LaunchIfMissing) { _Error = "Waiting for Visual Studio 2022 to finish loading or close a blocking dialog."; return false; }
    wchar_t ProgramFiles[MAX_PATH] = {};
    GetEnvironmentVariableW(L"ProgramFiles", ProgramFiles, MAX_PATH);
    for (const auto* Edition : {L"Enterprise", L"Professional", L"Community"})
    {
        const auto Executable = std::filesystem::path(ProgramFiles) / L"Microsoft Visual Studio/2022" / Edition / L"Common7/IDE/devenv.exe";
        if (!std::filesystem::is_regular_file(Executable, Error)) continue;
        FString Command = L"\"" + Executable.wstring() + L"\" \"" + _Solution.wstring() + L"\"";
        STARTUPINFOW Startup = {}; Startup.cb = sizeof(Startup);
        wil::unique_process_information Process;
        if (CreateProcessW(Executable.c_str(), &Command[0], nullptr, nullptr, FALSE, 0, nullptr, _Solution.parent_path().c_str(), &Startup, Process.addressof()))
        { _Error = "Starting Visual Studio 2022..."; return false; }
    }
    _Error = "Could not open Visual Studio 2022. Check its installation or close any blocking dialog.";
    return false;
}
