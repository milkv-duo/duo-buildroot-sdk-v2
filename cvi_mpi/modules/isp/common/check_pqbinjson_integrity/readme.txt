1.architecture
isp_json_struct.c --
                    \
                     -----checkPqBinJsonIntegrity.py---->show diff struct num and diff struct member
                    /
header file       --
1) find isp_json_struct.c  Adict={"structName":[structMemberNum,[structMemberList]]}
2) find header file Bdict={"structName":[structMemberNum,[structMemberList]]}
3) find intersection between Adict and Bdict
4) Bdict = Bset - (Aset & Bset)
5) find Adict and Bdict structMemberNum not match
6) show log
