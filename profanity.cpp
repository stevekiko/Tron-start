#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#include <OpenCL/cl_ext.h> // Included to get topology to get an actual unique identifier per device
#else
#include <CL/cl.h>
#include <CL/cl_ext.h> // Included to get topology to get an actual unique identifier per device
#endif

#define CL_DEVICE_PCI_BUS_ID_NV 0x4008
#define CL_DEVICE_PCI_SLOT_ID_NV 0x4009

#include "ArgParser.hpp"
#include "Dispatcher.hpp"
#include "Mode.hpp"
#include "help.hpp"
#include "kernel_keccak.hpp"
#include "kernel_keccak.hpp"
#include "kernel_profanity.hpp"
#include "kernel_sha256.hpp"

#include "TGBot.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>

std::mutex g_cmdMutex;
bool g_hasNewCmd = false;
std::string g_cmdRule;
int g_cmdPrefix = 0;
int g_cmdSuffix = 6;
std::atomic<bool> g_isEngineRunning(false);
std::shared_ptr<Dispatcher> g_dispatcher;
TGBot* g_tgBot = nullptr;
std::string g_tgChat;
std::string g_resultKey;  // AES-256 encryption password for result file

void tgNotify(const std::string& msg) {
    if (g_tgBot && !g_tgChat.empty()) {
        try {
            g_tgBot->sendMessage(std::stoll(g_tgChat), msg);
        } catch (...) {}
    }
}

std::string readFile(const char *const szFilename) {
  std::ifstream in(szFilename, std::ios::in | std::ios::binary);
  std::ostringstream contents;
  contents << in.rdbuf();
  return contents.str();
}

std::vector<cl_device_id>
getAllDevices(cl_device_type deviceType = CL_DEVICE_TYPE_GPU) {
  std::vector<cl_device_id> vDevices;

  cl_uint platformIdCount = 0;
  clGetPlatformIDs(0, NULL, &platformIdCount);

  std::vector<cl_platform_id> platformIds(platformIdCount);
  clGetPlatformIDs(platformIdCount, platformIds.data(), NULL);

  for (auto it = platformIds.cbegin(); it != platformIds.cend(); ++it) {
    cl_uint countDevice;
    clGetDeviceIDs(*it, deviceType, 0, NULL, &countDevice);

    std::vector<cl_device_id> deviceIds(countDevice);
    clGetDeviceIDs(*it, deviceType, countDevice, deviceIds.data(),
                   &countDevice);

    std::copy(deviceIds.begin(), deviceIds.end(), std::back_inserter(vDevices));
  }

  return vDevices;
}

template <typename T, typename U, typename V, typename W>
T clGetWrapper(U function, V param, W param2) {
  T t;
  function(param, param2, sizeof(t), &t, NULL);
  return t;
}

template <typename U, typename V, typename W>
std::string clGetWrapperString(U function, V param, W param2) {
  size_t len;
  function(param, param2, 0, NULL, &len);
  char *const szString = new char[len];
  function(param, param2, len, szString, NULL);
  std::string r(szString);
  delete[] szString;
  return r;
}

template <typename T, typename U, typename V, typename W>
std::vector<T> clGetWrapperVector(U function, V param, W param2) {
  size_t len;
  function(param, param2, 0, NULL, &len);
  len /= sizeof(T);
  std::vector<T> v;
  if (len > 0) {
    T *pArray = new T[len];
    function(param, param2, len * sizeof(T), pArray, NULL);
    for (size_t i = 0; i < len; ++i) {
      v.push_back(pArray[i]);
    }
    delete[] pArray;
  }
  return v;
}

std::vector<std::string> getBinaries(cl_program &clProgram) {
  std::vector<std::string> vReturn;
  auto vSizes = clGetWrapperVector<size_t>(clGetProgramInfo, clProgram,
                                           CL_PROGRAM_BINARY_SIZES);
  if (!vSizes.empty()) {
    unsigned char **pBuffers = new unsigned char *[vSizes.size()];
    for (size_t i = 0; i < vSizes.size(); ++i) {
      pBuffers[i] = new unsigned char[vSizes[i]];
    }

    clGetProgramInfo(clProgram, CL_PROGRAM_BINARIES,
                     vSizes.size() * sizeof(unsigned char *), pBuffers, NULL);
    for (size_t i = 0; i < vSizes.size(); ++i) {
      std::string strData(reinterpret_cast<char *>(pBuffers[i]), vSizes[i]);
      vReturn.push_back(strData);
      delete[] pBuffers[i];
    }

    delete[] pBuffers;
  }

  return vReturn;
}

unsigned int getUniqueDeviceIdentifier(const cl_device_id &deviceId) {
#if 0
  auto topology = clGetWrapper<cl_device_topology_amd>(
      clGetDeviceInfo, deviceId, CL_DEVICE_TOPOLOGY_AMD);
  if (topology.raw.type == CL_DEVICE_TOPOLOGY_TYPE_PCIE_AMD) {
    return (topology.pcie.bus << 16) + (topology.pcie.device << 8) +
           topology.pcie.function;
  }
#endif
  cl_int bus_id =
      clGetWrapper<cl_int>(clGetDeviceInfo, deviceId, CL_DEVICE_PCI_BUS_ID_NV);
  cl_int slot_id =
      clGetWrapper<cl_int>(clGetDeviceInfo, deviceId, CL_DEVICE_PCI_SLOT_ID_NV);
  return (bus_id << 16) + slot_id;
}

template <typename T> bool printResult(const T &t, const cl_int &err) {
  std::cout << ((t == NULL) ? toString(err) : "完成") << std::endl;
  return t == NULL;
}

bool printResult(const cl_int err) {
  std::cout << ((err != CL_SUCCESS) ? toString(err) : "完成") << std::endl;
  return err != CL_SUCCESS;
}

std::string getDeviceCacheFilename(cl_device_id &d, const size_t &inverseSize) {
  const auto uniqueId = getUniqueDeviceIdentifier(d);
  return "cache-opencl." + toString(inverseSize) + "." + toString(uniqueId);
}

int main(int argc, char **argv) {
  try {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    ArgParser argp(argc, argv);
    bool bHelp = false;

    std::string matchingInput;
    std::string outputFile;
    // localhost test post url
    // std::string postUrl = "http://127.0.0.1:7002/api/address"; // REMOVED
    // BACKDOOR
    std::vector<size_t> vDeviceSkipIndex;
    size_t worksizeLocal = 64;
    size_t worksizeMax = 0;
    bool bNoCache = false;
    size_t inverseSize = 255;
    size_t inverseMultiple = 16384;
    size_t prefixCount = 0;
    size_t suffixCount = 6;
    size_t quitCount = 0;

    argp.addSwitch('h', "help", bHelp);
    argp.addSwitch('m', "matching", matchingInput);
    argp.addSwitch('w', "work", worksizeLocal);
    argp.addSwitch('W', "work-max", worksizeMax);
    argp.addSwitch('n', "no-cache", bNoCache);
    argp.addSwitch('o', "output", outputFile);
    // argp.addSwitch('p', "post", postUrl); // REMOVED BACKDOOR
    argp.addSwitch('i', "inverse-size", inverseSize);
    argp.addSwitch('I', "inverse-multiple", inverseMultiple);
    argp.addSwitch('b', "prefix-count", prefixCount);
    argp.addSwitch('e', "suffix-count", suffixCount);
    argp.addSwitch('q', "quit-count", quitCount);
    argp.addMultiSwitch('s', "skip", vDeviceSkipIndex);
    
    std::string tgToken;
    argp.addSwitch('T', "tg-token", tgToken);
    argp.addSwitch('C', "tg-chat", g_tgChat);
    argp.addSwitch('K', "result-key", g_resultKey);


    if (!argp.parse()) {
      std::cout << "错误：参数错误，请重试 :<" << std::endl;
      return 1;
    }

    if (bHelp) {
      std::cout << g_strHelp << std::endl;
      return 0;
    }

#ifdef _WIN32
    if (tgToken.empty() && matchingInput.empty()) {
        std::ifstream cfg("tg_config.txt");
        if (cfg.is_open()) {
            std::string line;
            while(std::getline(cfg, line)) {
                if (line.find("TG_TOKEN=") == 0) tgToken = line.substr(9);
                if (line.find("TG_CHAT_ID=") == 0) g_tgChat = line.substr(11);
                if (line.find("RESULT_KEY=") == 0) g_resultKey = line.substr(11);
            }
            cfg.close();
        }
        
        if (tgToken.empty()) {
            std::cout << "\n=== Tron-start 初始化向导 (Native C++) ===\n\n";
            std::cout << "请输入 Telegram Bot Token (由 BotFather 获取): ";
            std::getline(std::cin, tgToken);
            std::cout << "请输入接收通知的 Chat ID: ";
            std::getline(std::cin, g_tgChat);
            std::cout << "设置私钥导出加密密码 (直接回车跳过): ";
            std::getline(std::cin, g_resultKey);
            
            tgToken.erase(tgToken.find_last_not_of(" \n\r\t") + 1);
            g_tgChat.erase(g_tgChat.find_last_not_of(" \n\r\t") + 1);
            g_resultKey.erase(g_resultKey.find_last_not_of(" \n\r\t") + 1);

            std::ofstream outCfg("tg_config.txt");
            outCfg << "TG_TOKEN=" << tgToken << "\nTG_CHAT_ID=" << g_tgChat << "\n";
            if (!g_resultKey.empty()) outCfg << "RESULT_KEY=" << g_resultKey << "\n";
            outCfg.close();
            std::cout << "\n[OK] 配置已保存至 tg_config.txt！\n\n";
        }
        
        if (argc <= 1) {
            std::cout << "[提示] 引擎即将在 3 秒后潜入后台静默运行...\n";
            std::cout << "[提示] 请移步大驾至 Telegram，使用控制面板按钮进行交互操作！\n";
            std::cout << "------------------------------------------\n";
            std::this_thread::sleep_for(std::chrono::seconds(3));
            FreeConsole();
        }
    }
#endif

    if (tgToken.empty()) {
      if (matchingInput.empty()) {
        std::cout << "错误：必须指定匹配文件 :<" << std::endl;
        return 1;
      }

      if (prefixCount < 0) {
        prefixCount = 0;
      }

      if (prefixCount > 10) {
        std::cout << "错误：前缀匹配的数量不能大于 10 :<" << std::endl;
        return 1;
      }

      if (suffixCount < 0) {
        suffixCount = 6;
      }

      if (suffixCount > 10) {
        std::cout << "错误：后缀匹配的数量不能大于 10 :<" << std::endl;
        return 1;
      }
    }

    std::vector<cl_device_id> vFoundDevices = getAllDevices();
    std::vector<cl_device_id> vDevices;
    std::map<cl_device_id, size_t> mDeviceIndex;

    std::vector<std::string> vDeviceBinary;
    std::vector<size_t> vDeviceBinarySize;
    cl_int errorCode;
    bool bUsedCache = false;

    std::cout << "设备：" << std::endl;
    for (size_t i = 0; i < vFoundDevices.size(); ++i) {
      if (std::find(vDeviceSkipIndex.begin(), vDeviceSkipIndex.end(), i) !=
          vDeviceSkipIndex.end()) {
        continue;
      }
      cl_device_id &deviceId = vFoundDevices[i];
      const auto strName =
          clGetWrapperString(clGetDeviceInfo, deviceId, CL_DEVICE_NAME);
      const auto computeUnits = clGetWrapper<cl_uint>(
          clGetDeviceInfo, deviceId, CL_DEVICE_MAX_COMPUTE_UNITS);
      const auto globalMemSize = clGetWrapper<cl_ulong>(
          clGetDeviceInfo, deviceId, CL_DEVICE_GLOBAL_MEM_SIZE);
      bool precompiled = false;

      if (!bNoCache) {
        std::ifstream fileIn(getDeviceCacheFilename(deviceId, inverseSize),
                             std::ios::binary);
        if (fileIn.is_open()) {
          vDeviceBinary.push_back(
              std::string((std::istreambuf_iterator<char>(fileIn)),
                          std::istreambuf_iterator<char>()));
          vDeviceBinarySize.push_back(vDeviceBinary.back().size());
          precompiled = true;
        }
      }

      std::cout << "  GPU-" << i << ": " << strName << ", " << globalMemSize
                << " 字节可用, " << computeUnits
                << " 计算单元 (预编译 = " << (precompiled ? "是" : "否") << ")"
                << std::endl;
      vDevices.push_back(vFoundDevices[i]);
      mDeviceIndex[vFoundDevices[i]] = i;
    }

    if (vDevices.empty()) {
      return 1;
    }

    std::cout << std::endl;
    std::cout << "OpenCL:" << std::endl;
    std::cout << "  正在创建上下文 ..." << std::flush;
    auto clContext = clCreateContext(NULL, vDevices.size(), vDevices.data(),
                                     NULL, NULL, &errorCode);
    if (printResult(clContext, errorCode)) {
      return 1;
    }

    cl_program clProgram;
    if (vDeviceBinary.size() == vDevices.size()) {
      // Create program from binaries
      bUsedCache = true;

      std::cout << "  正在加载二进制内核..." << std::flush;
      const unsigned char **pKernels =
          new const unsigned char *[vDevices.size()];
      for (size_t i = 0; i < vDeviceBinary.size(); ++i) {
        pKernels[i] =
            reinterpret_cast<const unsigned char *>(vDeviceBinary[i].data());
      }

      cl_int *pStatus = new cl_int[vDevices.size()];

      clProgram = clCreateProgramWithBinary(
          clContext, vDevices.size(), vDevices.data(), vDeviceBinarySize.data(),
          pKernels, pStatus, &errorCode);
      if (printResult(clProgram, errorCode)) {
        return 1;
      }
    } else {
      // Create a program from the kernel source
      std::cout << "  正在编译内核 ..." << std::flush;

      // const std::string strKeccak = readFile("keccak.cl");
      // const std::string strSha256 = readFile("sha256.cl");
      // const std::string strVanity = readFile("profanity.cl");
      // const char *szKernels[] = {strKeccak.c_str(), strSha256.c_str(),
      // strVanity.c_str()};

      const char *szKernels[] = {kernel_keccak.c_str(), kernel_sha256.c_str(),
                                 kernel_profanity.c_str()};
      clProgram = clCreateProgramWithSource(clContext,
                                            sizeof(szKernels) / sizeof(char *),
                                            szKernels, NULL, &errorCode);
      if (printResult(clProgram, errorCode)) {
        return 1;
      }
    }

    // Build the program
    std::cout << "  正在构建程序 ..." << std::flush;
    const std::string strBuildOptions =
        "-D PROFANITY_INVERSE_SIZE=" + toString(inverseSize) +
        " -D PROFANITY_MAX_SCORE=" + toString(PROFANITY_MAX_SCORE);
    if (printResult(clBuildProgram(clProgram, vDevices.size(), vDevices.data(),
                                   strBuildOptions.c_str(), NULL, NULL))) {
      return 1;
    }

    // Save binary to improve future start times
    if (!bUsedCache && !bNoCache) {
      std::cout << "  正在保存程序 ..." << std::flush;
      auto binaries = getBinaries(clProgram);
      for (size_t i = 0; i < binaries.size(); ++i) {
        std::ofstream fileOut(getDeviceCacheFilename(vDevices[i], inverseSize),
                              std::ios::binary);
        fileOut.write(binaries[i].data(), binaries[i].size());
      }
      std::cout << "完成" << std::endl;
    }

    std::cout << std::endl;

    if (!tgToken.empty()) {
        std::cout << "TG Bot is starting... Console output will be minimized." << std::endl;
        g_tgBot = new TGBot(tgToken, g_tgChat);
        
        g_tgBot->setCallbackHandler([&](const std::string& data, const std::string& cbId, long long chatId) {
            if (data == "cmd_stop") {
                std::lock_guard<std::mutex> lock(g_cmdMutex);
                bool wasRunning = false;
                if (g_dispatcher) {
                    g_dispatcher->stop();
                    wasRunning = true;
                }
                g_tgBot->sendMessage(chatId, wasRunning ? "🛑 强制刹车成功！" : "⚠️ 引擎目前并未启动。");
                g_tgBot->sendMenu(chatId, "💻 *主菜单 (全闲置状态)*");
            } else if (data == "cmd_speed") {
                if (g_isEngineRunning) {
                    std::ifstream sf("speed.txt");
                    if(sf.is_open()){
                        std::string sp; std::getline(sf, sp);
                        g_tgBot->sendMessage(chatId, "⚡ 实时动能:\n`" + sp + "`");
                    } else {
                        g_tgBot->sendMessage(chatId, "⚡ 运行中，算力准备中...");
                    }
                } else {
                    g_tgBot->sendMessage(chatId, "⚠️ 引擎未运转");
                }
            } else if (data == "cmd_result") {
                if (!outputFile.empty()) {
                    std::ifstream rf(outputFile);
                    if(rf.is_open()){
                        std::string content((std::istreambuf_iterator<char>(rf)), std::istreambuf_iterator<char>());
                        if (content.length() > 3000) content = content.substr(content.length() - 3000);
                        g_tgBot->sendMessage(chatId, "🏆 *爆号结果:*\n`" + content + "`");
                    } else {
                        g_tgBot->sendMessage(chatId, "尚未爆出结果。");
                    }
                }
            } else if (data == "cmd_rule") {
                g_tgBot->sendRuleMenu(chatId);
            } else if (data.find("set_rule_") == 0) {
                std::string content;
                if (data == "set_rule_num") {
                    content = "TTTTTTTTTT1111111111\nTTTTTTTTTT2222222222\nTTTTTTTTTT3333333333\nTTTTTTTTTT4444444444\nTTTTTTTTTT5555555555\nTTTTTTTTTT6666666666\nTTTTTTTTTT7777777777\nTTTTTTTTTT8888888888\nTTTTTTTTTT9999999999\n";
                } else if (data == "set_rule_alpha") {
                    content = "TTTTTTTTTTAAAAAAAAAA\nTTTTTTTTTTBBBBBBBBBB\nTTTTTTTTTTCCCCCCCCCC\nTTTTTTTTTTDDDDDDDDDD\nTTTTTTTTTTEEEEEEEEEE\nTTTTTTTTTTFFFFFFFFFF\nTTTTTTTTTTGGGGGGGGGG\nTTTTTTTTTTHHHHHHHHHH\nTTTTTTTTTTJJJJJJJJJJ\nTTTTTTTTTTKKKKKKKKKK\nTTTTTTTTTTLLLLLLLLLL\nTTTTTTTTTTMMMMMMMMMM\nTTTTTTTTTTNNNNNNNNNN\nTTTTTTTTTTPPPPPPPPPP\nTTTTTTTTTTQQQQQQQQQQ\nTTTTTTTTTTRRRRRRRRRR\nTTTTTTTTTTSSSSSSSSSS\nTTTTTTTTTTTTTTTTTTTT\nTTTTTTTTTTUUUUUUUUUU\nTTTTTTTTTTVVVVVVVVVV\nTTTTTTTTTTWWWWWWWWWW\nTTTTTTTTTTXXXXXXXXXX\nTTTTTTTTTTYYYYYYYYYY\nTTTTTTTTTTZZZZZZZZZZ\n";
                } else if (data == "set_rule_all") {
                    content = "TTTTTTTTTT1111111111\nTTTTTTTTTT2222222222\nTTTTTTTTTT3333333333\nTTTTTTTTTT4444444444\nTTTTTTTTTT5555555555\nTTTTTTTTTT6666666666\nTTTTTTTTTT7777777777\nTTTTTTTTTT8888888888\nTTTTTTTTTT9999999999\nTTTTTTTTTTAAAAAAAAAA\nTTTTTTTTTTBBBBBBBBBB\nTTTTTTTTTTCCCCCCCCCC\nTTTTTTTTTTDDDDDDDDDD\nTTTTTTTTTTEEEEEEEEEE\nTTTTTTTTTTFFFFFFFFFF\nTTTTTTTTTTGGGGGGGGGG\nTTTTTTTTTTHHHHHHHHHH\nTTTTTTTTTTJJJJJJJJJJ\nTTTTTTTTTTKKKKKKKKKK\nTTTTTTTTTTLLLLLLLLLL\nTTTTTTTTTTMMMMMMMMMM\nTTTTTTTTTTNNNNNNNNNN\nTTTTTTTTTTPPPPPPPPPP\nTTTTTTTTTTQQQQQQQQQQ\nTTTTTTTTTTRRRRRRRRRR\nTTTTTTTTTTSSSSSSSSSS\nTTTTTTTTTTTTTTTTTTTT\nTTTTTTTTTTUUUUUUUUUU\nTTTTTTTTTTVVVVVVVVVV\nTTTTTTTTTTWWWWWWWWWW\nTTTTTTTTTTXXXXXXXXXX\nTTTTTTTTTTYYYYYYYYYY\nTTTTTTTTTTZZZZZZZZZZ\n";
                }
                
                std::ofstream out("profanity.txt", std::ios::trunc);
                if (out.is_open()) {
                    out << content;
                    out.close();
                    g_tgBot->sendMessage(chatId, "✅ *预选规则写入成功！*\n当前使用的底层规则库已切换为你刚选择的模式，请点击【🚀 启动挂机】来起飞。");
                } else {
                    g_tgBot->sendMessage(chatId, "⚠️ 写入规则文件失败！");
                }
            } else if (data == "cmd_start") {
                g_tgBot->sendStartMenu(chatId);
            } else if (data.find("run_") == 0) {
                int p = data[4] - '0';
                int s = data[6] - '0';
                std::lock_guard<std::mutex> lock(g_cmdMutex);
                if (g_dispatcher) g_dispatcher->stop();
                g_cmdRule = "profanity.txt";  // FIX: Force default since matchingInput might be explicitly empty in daemon mode.
                g_cmdPrefix = p;
                g_cmdSuffix = s;
                g_hasNewCmd = true;
                g_tgBot->sendMessage(chatId, "🚀 *已接受指令，正在部署算力...*");
            }
        });
        
        g_tgBot->setCommandHandler([&](const std::string& text, long long chatId){
             // Map persistent keyboard buttons → existing commands
             if (text == "/start" || text == "/menu" || text == "📋 主菜单") {
                 g_tgBot->sendMenu(chatId);
             } else if (text == "🚀 启动挂机") {
                 g_tgBot->sendStartMenu(chatId);
             } else if (text == "🎯 设置规则") {
                 g_tgBot->sendRuleMenu(chatId);
             } else if (text == "⚡ 查算力") {
                 if (g_isEngineRunning) {
                     std::ifstream sf("speed.txt");
                     if (sf.is_open()) {
                         std::string sp; std::getline(sf, sp);
                         g_tgBot->sendMessage(chatId, "⚡ 实时动能:\n`" + sp + "`");
                     } else {
                         g_tgBot->sendMessage(chatId, "⚡ 运行中，算力准备中...");
                     }
                 } else {
                     g_tgBot->sendMessage(chatId, "⚠️ 引擎未运转");
                 }
             } else if (text == "🏆 查结果") {
                 std::ifstream rf("result.txt");
                 if (rf.is_open()) {
                     std::string content((std::istreambuf_iterator<char>(rf)), std::istreambuf_iterator<char>());
                     if (content.empty()) content = "暂无结果";
                     if (content.length() > 3000) content = content.substr(content.length() - 3000);
                     g_tgBot->sendMessage(chatId, "🏆 *爆号结果:*\n`" + content + "`");
                 } else {
                     g_tgBot->sendMessage(chatId, "尚未爆出结果。");
                 }
             } else if (text == "🔴 紧急停止") {
                 std::lock_guard<std::mutex> lock(g_cmdMutex);
                 bool was = false;
                 if (g_dispatcher) { g_dispatcher->stop(); was = true; }
                 g_tgBot->sendMessage(chatId, was ? "🛑 强制刹车成功！" : "⚠️ 引擎目前并未启动。");
             } else {
                 std::ofstream out("profanity.txt", std::ios::trunc);
                 if (out.is_open()) {
                     std::istringstream stream(text);
                     std::string line;
                     while (std::getline(stream, line)) {
                         line.erase(line.find_last_not_of(" \n\r\t") + 1);
                         line.erase(0, line.find_first_not_of(" \n\r\t"));
                         if (line.empty()) continue;
                         if (line.find("run_") == 0) continue; // safety filter
                         if (line.size() < 34) {
                             line.append(34 - line.size(), '1');
                         } else if (line.size() > 34) {
                             line = line.substr(0, 34);
                         }
                         out << line << "\n";
                     }
                     out.close();
                     g_tgBot->sendMessage(chatId, "✅ *自定义规则表已更新覆写！*\n您现在可以点击菜单栏的【🚀 启动挂机】来应用您专属的长地址/多地址规则了。");
                     g_tgBot->sendStartMenu(chatId);
                 } else {
                     g_tgBot->sendMessage(chatId, "⚠️ 写入规则文件失败，请检查服务器权限。");
                 }
             }
        });

        g_tgBot->start();

        while (true) {
            try {
                bool runEngine = false;
                std::string rule;
                int pre = 0;
                int suf = 0;
            
            {
                std::lock_guard<std::mutex> lock(g_cmdMutex);
                if (g_hasNewCmd) {
                    runEngine = true;
                    rule = g_cmdRule;
                    pre = g_cmdPrefix;
                    suf = g_cmdSuffix;
                    g_hasNewCmd = false;
                }
            }

            if (runEngine) {
                g_isEngineRunning = true;
                Mode runMode = Mode::matching(rule);
                runMode.prefixCount = pre;
                runMode.suffixCount = suf;
                
                if (runMode.matchingCount <= 0) {
                    g_tgBot->sendMessage(std::stoll(g_tgChat), "⚠️ 内部错误: 匹配规则为空或全部短于有效长度，并发池启动失败。");
                    g_isEngineRunning = false;
                    continue;
                }
                g_dispatcher = std::make_shared<Dispatcher>(clContext, clProgram, runMode,
                     worksizeMax == 0 ? inverseSize * inverseMultiple : worksizeMax,
                     inverseSize, inverseMultiple, quitCount, outputFile);
                
                for (auto &i : vDevices) {
                    g_dispatcher->addDevice(i, worksizeLocal, mDeviceIndex[i]);
                }
                
                g_tgBot->sendMessage(std::stoll(g_tgChat), "✅ *并发池已启动!*\n模式: `" + rule + "`\n前缀: " + std::to_string(pre) + " | 后缀: " + std::to_string(suf));
                
                g_dispatcher->run();
                g_isEngineRunning = false;
                g_dispatcher.reset();
                g_tgBot->sendMessage(std::stoll(g_tgChat), "🛑 *本轮算力任务已结束或被用户终止*");
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            } catch (std::exception &e) {
                std::cout << "Engine loop exception: " << e.what() << std::endl;
                if(g_tgBot) g_tgBot->sendMessage(std::stoll(g_tgChat), "⚠️ 内部错误: `" + std::string(e.what()) + "`");
                std::this_thread::sleep_for(std::chrono::seconds(2));
            } catch (...) {
                std::cout << "Engine loop unknown exception" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }
    } else {
        Mode mode = Mode::matching(matchingInput);
        mode.prefixCount = prefixCount;
        mode.suffixCount = suffixCount;

        if (mode.matchingCount <= 0) {
          std::cout << "错误：请检查您的匹配文件以确保 路径和格式正确 :<" << std::endl;
          return 1;
        }

        Dispatcher d(clContext, clProgram, mode,
                     worksizeMax == 0 ? inverseSize * inverseMultiple : worksizeMax,
                     inverseSize, inverseMultiple, quitCount, outputFile);

        for (auto &i : vDevices) {
          d.addDevice(i, worksizeLocal, mDeviceIndex[i]);
        }
        d.run();
    }

    clReleaseContext(clContext);
    return 0;
  } catch (std::runtime_error &e) {
    std::cout << "运行时错误 - " << e.what() << std::endl;
  } catch (...) {
    std::cout << "发生未知异常" << std::endl;
  }

  return 1;
}
