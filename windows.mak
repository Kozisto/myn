#	Build MSI packages

MSB=%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\amd64\msbuild.exe

WIX_LINK = light.exe -ext WixUIExtension -ext WixUtilExtension -dWixUILicenseRtf=doc\mynt_lic.rtf -dWixUIDialogBmp=share\pixmaps\wix-banner.bmp -dWixUIBannerBmp=share\pixmaps\wix-topbanner.bmp


all : mynt-2.16.0-win32.msi mynt-2.16.0-win64.msi

src\qt\res_bitcoin.cpp : src\qt\moc.proj
	cd src\qt
	msbuild moc.proj
	cd ..\..


x86_R_St\mynt-qt.exe : src\mynt.cpp src\qt\res_bitcoin.cpp
	""$(MSB)"" my-mynt.sln /p:Configuration=R_St,Platform=x86 /v:n

x64_R_St\mynt-qt.exe : src\mynt.cpp src\qt\res_bitcoin.cpp
	""$(MSB)"" my-mynt.sln /p:Configuration=R_St,Platform=x64 /v:n


mynt-2.16.0-win32.msi : mynt.wxs x86_R_St\mynt-qt.exe
	candle.exe -o mynt-x86.wixobj mynt.wxs
	$(WIX_LINK)  -out $@ mynt-x86.wixobj

mynt-2.16.0-win64.msi : mynt.wxs x64_R_St\mynt-qt.exe
	candle.exe -arch x64 -o mynt-x64.wixobj mynt.wxs
	$(WIX_LINK) -out $@ mynt-x64.wixobj
