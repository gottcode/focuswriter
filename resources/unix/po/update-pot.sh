#!/bin/sh


echo -n 'Preparing files...'
cd ..

rm -f focuswriter.desktop.in
cp focuswriter.desktop focuswriter.desktop.in
sed -e '/^Name\[/ d' \
	-e '/^GenericName\[/ d' \
	-e '/^Comment\[/ d' \
	-e '/^Icon/ d' \
	-e '/^Keywords/ d' \
	-i focuswriter.desktop.in

rm -f focuswriter.appdata.xml.in
cp focuswriter.appdata.xml focuswriter.appdata.xml.in
sed -e '/p xml:lang/ d' \
	-e '/summary xml:lang/ d' \
	-e '/name xml:lang/ d' \
	-e '/<developer_name>/ d' \
	-i focuswriter.appdata.xml.in

cd po
echo ' DONE'


echo -n 'Extracting messages...'
xgettext --from-code=UTF-8 --output=description.pot \
	--package-name='FocusWriter' --copyright-holder='Graeme Gott' \
	../*.in
sed 's/CHARSET/UTF-8/' -i description.pot
echo ' DONE'


echo -n 'Cleaning up...'
cd ..

rm -f focuswriter.desktop.in
rm -f focuswriter.appdata.xml.in

echo ' DONE'
