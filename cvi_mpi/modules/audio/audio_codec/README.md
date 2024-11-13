#This folder provide:
#1. actual source code of audio codec
#2. build lib process of the codec: adpcm/g711/g726
#3. test pattern or test program of the cvi_transcode

#source code origin:
g711/g726 from easyAAC:
https://github.com/EasyDarwin/EasyAACEncoder
https://www.itread01.com/p/46993.html
http://www.easydarwin.org/

g711/g726 from baresip/spandsp:
https://github.com/jart/spandsp/blob/master/src/g711.c



ADPCM from spandsp source code:
https://github.com/jart/spandsp
please check the ima_adpcm.c to trace all the adpcm source
Also check the ima_adpcm_tests.c to check the usage

AAC encoder from easydarwin / easydarwin are from Faac in github
https://github.com/peterfuture/Faac
https://github.com/EasyDarwin/EasyAACEncoder

AAC decoder from below information:
1.faad2-2.7 source code is from official website:
  http://www.audiocoding.com/
2.The reference code are from below two websites:
 -https://github.com/sunqibuhuake/aac-wasm-decoder
 -https://github.com/liaoqingfu/aac-decoder
3.The bug and patch you should notice:
 -https://www.itdaan.com/tw/7446ef838daa




