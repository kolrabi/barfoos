#!/bin/sh

# Bundle information
BUNDLE=barfoos
BUNDLEID=de.kolrabi.barfoos
EXEC=barfoos

# Version information
MAJOR=0
MINOR=1

#######################################################################

# Set up paths
APP=$BUNDLE.app
APPCONTENTS=$APP/Contents
APPBIN=$APPCONTENTS/MacOS
APPEXEC=$APPBIN/$EXEC
VERSION=$MAJOR.$MINOR

# Build app bundle structure
rm -rf $APP
mkdir $APP $APPCONTENTS $APPCONTENTS/Resources $APPBIN
cp -r ../assets $APPCONTENTS/assets
cp $EXEC $APPBIN

# Rewire dynamic libraries
otool -L $EXEC | cut -f 2 | grep -v "^/System"|grep "^/"|while read LIB REST; do
  LIBFILE=`basename $LIB`
  cp $LIB $APPBIN
  install_name_tool -change $LIB @executable_path/$LIBFILE $APPEXEC
done

# Starter script
echo "#!/bin/bash"  >  $APPBIN/$EXEC.sh
echo 'cd "${0%/*}"' >> $APPBIN/$EXEC.sh
echo "./$EXEC"      >> $APPBIN/$EXEC.sh
chmod +x $APPBIN/$EXEC.sh

# Property list
cat > $APPCONTENTS/Info.plist << EOF
<?xml version="1.0" encoding="UTF-8"?>
!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleGetInfoString</key>          <string>$BUNDLE</string>
  <key>CFBundleExecutable</key>             <string>$EXEC.sh</string>
  <key>CFBundleIdentifier</key>             <string>$BUNDLEID</string>
  <key>CFBundleName</key>                   <string>$BUNDLE</string>
  <key>CFBundleIconFile</key>               <string>$BUNDLE.png</string>
  <key>CFBundleShortVersionString</key>     <string>$VERSION</string>
  <key>CFBundleInfoDictionaryVersion</key>  <string>6.0</string>
  <key>CFBundlePackageType</key>            <string>APPL</string>
  <key>IFMajorVersion</key>                 <integer>$MAJOR</integer>
  <key>IFMinorVersion</key>                 <integer>$MINOR</integer>
</dict>
</plist>
EOF

# Done :)
