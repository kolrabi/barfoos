#!/bin/sh

APP=barfoos.app
APPBIN=$APP/Contents/MacOS
EXEC=barfoos
APPEXEC=$APPBIN/$EXEC

rm -rf $APP

mkdir $APP $APP/Contents $APP/Contents/Resources $APPBIN
cp -r ../assets $APP/Contents/assets
cp $EXEC $APPBIN

otool -L $EXEC | cut -f 2 | grep -v "^/System"|grep "^/"|while read LIB REST; do

LIBFILE=`basename $LIB`
LIBDIR=`dirname $LIB`

cp $LIB $APPBIN
install_name_tool -change $LIB @executable_path/$LIBFILE $APPEXEC

done

echo "#!/bin/bash" > $APPBIN/$EXEC.sh
echo 'cd "${0%/*}"' >> $APPBIN/$EXEC.sh
echo "./$EXEC" >> $APPBIN/$EXEC.sh
chmod +x $APPBIN/$EXEC.sh

cat > $APP/Contents/Info.plist << EOF
<?xml version="1.0" encoding="UTF-8"?>
!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleGetInfoString</key>
  <string>Foo</string>
  <key>CFBundleExecutable</key>
  <string>$EXEC.sh</string>
  <key>CFBundleIdentifier</key>
  <string>com.your-company-name.www</string>
  <key>CFBundleName</key>
  <string>foo</string>
  <key>CFBundleIconFile</key>
  <string>foo.icns</string>
  <key>CFBundleShortVersionString</key>
  <string>0.01</string>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>IFMajorVersion</key>
  <integer>0</integer>
  <key>IFMinorVersion</key>
  <integer>1</integer>
</dict>
</plist>
EOF

