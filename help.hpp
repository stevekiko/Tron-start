#ifndef HPP_HELP
#define HPP_HELP

#include <string>

const std::string g_strHelp = R"(
Usage: ./profanity [OPTIONS]

  Help:
    --help              显示帮助信息

  Modes with arguments:
    --matching          指定匹配规则，可以是单个地址字符串，也可以是规则文件 (如 profanity.txt)

  Matching configuration:
    --prefix-count      最少匹配的前缀位数, 默认 0
    --suffix-count      最少匹配的后缀位数, 默认 6
    --quit-count        匹配到指定数量的地址后退出程序, 默认 0 (不退出)

  Device control:
    --skip              跳过指定索引的显卡设备 (Index)

  Output control:
    --output            将结果保存到指定文件

Examples:

  ./profanity --matching profanity.txt
  ./profanity --matching profanity.txt --skip 1
  ./profanity --matching profanity.txt --prefix-count 1 --suffix-count 8
  ./profanity --matching profanity.txt --prefix-count 1 --suffix-count 10 --quit-count 1
  ./profanity --matching profanity.txt --output result.txt
  ./profanity --matching TUqEg3dzVEJNQSVW2HY98z5X8SBdhmao8D --prefix-count 2 --suffix-count 4 --quit-count 1

About:

  Profanity is a vanity address generator for tron: https://tron.network/
  The software is modified based on ethereum profanity: https://github.com/johguse/profanity
  Please make sure the program you are running is download from: https://github.com/stevekiko/Tron-start
  Author: telegram -> @enyccd

安全警告 (Security Warning):

  在使用生成的靓号地址前，请务必进行私钥匹配验证。
  为了资产安全，建议始终对生成的地址进行多重签名 (Multi-sign) 处理。
)";

#endif /* HPP_HELP */
