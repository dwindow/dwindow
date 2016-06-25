!include "logiclib.nsh"
!ifndef Project
!define Project AssocTool   ;改成你的软件名称
!endif

;关联文件，用法：!insertmacro Assoc dts "dts" "DVD 文件" "MCP.exe" "MCP.ico"
!macro Assoc EXT TYPE DESC OPENEXE ICO
Push $0
Push $1
Push $2
Push $3

ReadRegStr $1 HKCR ".${EXT}" ""                 ;读现在的

${if} $1 == ""
  DetailPrint '$(Associatedfiletypes) *.${ext} $(For) "${desc}"...'
  WriteRegStr HKCR "Back.${Project}\.${ext}" "" "_Blank_"
${Else}
  DetailPrint '$(Modifyfiletypes) *.${ext} $(For) "${desc}"...'
  StrLen $3 ${Project}
  StrCpy $2 $1 $3
  ${if} $2 != "${Project}"
  	WriteRegStr HKCR "Back.${Project}\.${ext}" "" "$1"   ;如果不是我们关联的，备份该扩展名
  ${Endif}
${EndIf}
;关于基本关联的
WriteRegStr HKCR ".${ext}" "" "${Project}.${type}"
WriteRegStr HKCR "${Project}.${type}" "" "${desc}"
WriteRegStr HKCR "${Project}.${type}\shell" "" "open"
WriteRegStr HKCR "${Project}.${type}\shell\open" "" "$(PLAY)"
WriteRegStr HKCR "${Project}.${type}\shell\open\command" "" '${openexe} "%1"'
WriteRegStr HKCR "${Project}.${type}\DefaultIcon" "" "${ico}"
;标记有关联文件(只要非空即可)
WriteRegStr HKCR "Back.${Project}" "" "${Project} Backup"
Pop $3
Pop $2
Pop $1
!macroend

;取消关联，用法：!insertmacro UnAssoc dts
!macro UnAssoc EXT

Push $1
Push $2  ;Content Type
Push $3  ;CLSID
;**********修改以下代码要谨慎!**********

ReadRegStr $1 HKCR "Back.${Project}\.${EXT}" ""     ;读备份
ReadRegStr $2 HKCR ".${EXT}" ""                 ;读现在的

StrLen $3 ${Project}
StrCpy $4 $2 $3

${if} "$4" == "${Project}"        ;如果现在还是我们关联了
${if} "$1" == "_Blank_"  ;如果备份有"_Blank_",证明已经关联,但没有旧类型.
    WriteRegStr HKCR ".${EXT}" "" ""           ;不作处理
${Else}
    ${if} "$1" != ""
    WriteRegStr HKCR ".${EXT}" "" "$1"         ;恢复备份
    ${EndIf}
${EndIf}
DeleteRegKey HKCR "Back.${Project}\.${ext}"   ;恢复完成,删除备份
StrCmp "${Project}.${ext}" "." +2
DeleteRegKey HKCR "${Project}.${ext}"   ;恢复完成,删除备份
${EndIf}
Pop $3
Pop $2
Pop $1
!macroend