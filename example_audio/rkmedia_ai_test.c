// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Example AI (Audio Input) capture program for RKMedia API.
// 功能：从AI通道读取音频帧并保存到PCM文件（默认 /tmp/ai.pcm）。

#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "rkmedia_api.h"

// 全局退出标志，收到 SIGINT 后置 true 让线程停止
static bool quit = false;

static void sigterm_handler(int sig) {
  // 捕捉信号并打印，方便调试
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

// AI通道取帧并写入文件的线程函数
static void *GetMediaBuffer(void *path) {
  char *save_path = (char *)path;
  printf("#Start %s thread, arg:%s\n", __func__, save_path);

  // 以写方式打开输出PCM文件
  FILE *save_file = fopen(save_path, "w");
  if (!save_file)
    printf("ERROR: Open %s failed!\n", save_path);

  MEDIA_BUFFER mb = NULL;
  int cnt = 0;
  while (!quit) {
    // 从AI通道(0号)读取媒体缓冲区，超时-1表示无限等待
    mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_AI, 0, -1);
    if (!mb) {
      printf("RK_MPI_SYS_GetMediaBuffer get null buffer!\n");
      break;
    }

    // 打印帧信息，便于调试
    printf("#%d Get Frame:ptr:%p, size:%zu, mode:%d, channel:%d, timestamp:%lld\n",
           cnt++, RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb),
           RK_MPI_MB_GetModeID(mb), RK_MPI_MB_GetChannelID(mb),
           RK_MPI_MB_GetTimestamp(mb));

    // 将PCM数据写入文件
    if (save_file)
      fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), save_file);

    // 释放帧缓冲区内存
    RK_MPI_MB_ReleaseBuffer(mb);
  }

  if (save_file)
    fclose(save_file);

  return NULL;
}

// getopt参数串：支持 -d -r -c -s -o -v -f
static RK_CHAR optstr[] = "?::d:c:r:s:o:v:f:";

// 帮助信息输出函数
static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
  printf("\t%s [-d default] [-r 16000] [-c 2] [-s 1024] -o /tmp/ai.pcm\n", name);
  printf("\t-d: device name, Default:\"default\"\n");
  printf("\t-r: sample rate, Default:16000\n");
  printf("\t-c: channel count, Default:2\n");
  printf("\t-s: frames cnt, Default:1024\n");
  printf("\t-f: the fmt of AI, 0:u8 1:s16 2:s32 3:flt 4:u8p 5:s16p 6:s32p 7:fltp 8:g711a 9:g711u Default:s16\n");
  printf("\t-v: volume, Default:50 (0-100)\n");
  printf("\t-o: output path, Default:\"/tmp/ai.pcm\"\n");
  printf("Notice: fmt always s16_le\n");
}

int main(int argc, char *argv[]) {
  // 默认参数
  RK_U32 u32SampleRate = 16000;
  RK_U32 u32ChnCnt = 2;
  RK_U32 u32FrameCnt = 1024;
  RK_S32 s32Volume = 50;
  RK_CHAR *pDeviceName = "default";   // AI设备节点，默认为 alsa default
  RK_CHAR *pOutPath = "/tmp/ai.pcm"; // 输出PCM文件路径
  SAMPLE_FORMAT_E enSampleFmt = RK_SAMPLE_FMT_S16;
  int c;
  int ret = 0;

  // 解析命令行参数，覆盖默认配置
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'd':
      pDeviceName = optarg;
      break;
    case 'r':
      u32SampleRate = atoi(optarg);
      break;
    case 'c':
      u32ChnCnt = atoi(optarg);
      break;
    case 's':
      u32FrameCnt = atoi(optarg);
      break;
    case 'v':
      s32Volume = atoi(optarg);
      break;
    case 'o':
      pOutPath = optarg;
      break;
    case 'f':
      enSampleFmt = atoi(optarg);
      break;
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  // 打印当前配置
  printf("#Device: %s\n", pDeviceName);
  printf("#SampleRate: %u\n", u32SampleRate);
  printf("#Channel Count: %u\n", u32ChnCnt);
  printf("#Frame Count: %u\n", u32FrameCnt);
  printf("#Volume: %d\n", s32Volume);
  printf("#Output Path: %s\n", pOutPath);
  printf("#SampleFmt: %d\n", enSampleFmt);

  // 1) 初始化RKMedia系统
  RK_MPI_SYS_Init();

  // 2) 配置AI通道属性
  AI_CHN_ATTR_S ai_attr;
  ai_attr.pcAudioNode = pDeviceName;
  ai_attr.enSampleFormat = enSampleFmt;
  ai_attr.u32NbSamples = u32FrameCnt;
  ai_attr.u32SampleRate = u32SampleRate;
  ai_attr.u32Channels = u32ChnCnt;
  ai_attr.enAiLayout = AI_LAYOUT_NORMAL;

  // 3) 设置AI通道并使能
  ret = RK_MPI_AI_SetChnAttr(0, &ai_attr);
  ret |= RK_MPI_AI_EnableChn(0);
  if (ret) {
    printf("Enable AI[0] failed! ret=%d\n", ret);
    return -1;
  }

  // 4) 查询当前音量和设置目标音量
  RK_S32 s32CurrentVolmue = 0;
  ret = RK_MPI_AI_GetVolume(0, &s32CurrentVolmue);
  if (ret) {
    printf("Get Volume(before) failed! ret=%d\n", ret);
    return -1;
  }
  printf("#Before Volume set, volume=%d\n", s32CurrentVolmue);

  ret = RK_MPI_AI_SetVolume(0, s32Volume);
  if (ret) {
    printf("Set Volume failed! ret=%d\n", ret);
    return -1;
  }

  s32CurrentVolmue = 0;
  ret = RK_MPI_AI_GetVolume(0, &s32CurrentVolmue);
  if (ret) {
    printf("Get Volume(after) failed! ret=%d\n", ret);
    return -1;
  }
  printf("#After Volume set, volume=%d\n", s32CurrentVolmue);

  // 5) 启动读数据线程和AI数据流
  pthread_t read_thread;
  pthread_create(&read_thread, NULL, GetMediaBuffer, pOutPath);

  ret = RK_MPI_AI_StartStream(0);
  if (ret) {
    printf("Start AI failed! ret=%d\n", ret);
    return -1;
  }

  printf("%s initial finish\n", __func__);

  // 注册信号处理，收到 SIGINT 退出循环并停止采集
  signal(SIGINT, sigterm_handler);

  while (!quit) {
    usleep(500000); // 0.5秒休眠，降低CPU占用
  }

  printf("%s exit!\n", __func__);

  // 6) 关闭AI通道
  RK_MPI_AI_DisableChn(0);

  return 0;
}
