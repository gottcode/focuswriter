#!/bin/bash


# Clean up previous run
rm -rf dicts

# Work in temporary directory
mkdir temp
cd temp


# Download
echo -n 'Downloading LibreOffice dictionaries...'
loversion='6.0.0.3'
lodict="libreoffice-dictionaries-${loversion}"
lofiles="libreoffice-${loversion}/dictionaries"
if [ ! -e "${lodict}.tar.xz" ]; then
	curl -s -O -L "https://download.documentfoundation.org/libreoffice/src/6.0.0/${lodict}.tar.xz"
	echo ' DONE'
else
	echo ' SKIPPED'
fi

echo -n 'Downloading Armenian dictionary...'
hydict='hy_am_e_1940_dict-1.1'
hyfiles='hy_am_e_1940_dict-1.1'
if [ ! -e "${hydict}.oxt" ]; then
	curl -s -L -o "${hydict}.oxt" 'https://extensions.openoffice.org/en/download/4838'
	echo ' DONE'
else
	echo ' SKIPPED'
fi

echo -n 'Downloading Esperanto dictionary...'
eodict='esperantilo'
eofiles='esperantilo'
if [ ! -e "${eodict}.oxt" ]; then
	curl -s -L -o "${eodict}.oxt" 'https://extensions.openoffice.org/en/download/4561'
	echo ' DONE'
else
	echo ' SKIPPED'
fi

echo -n 'Downloading Finnish dictionary...'
if [ ! -e 'voikko.oxt' ]; then
	curl -s -O 'https://www.puimula.org/htp/ooo/voikko-win/5.0.0.20151123/voikko.oxt'
	echo ' DONE'
else
	echo ' SKIPPED'
fi

echo -n 'Downloading Korean dictionary...'
kodict='Korean_spell-checker-0.6.0-1_LibO'
kofiles='Korean_spell-checker-0.6.0-1_LibO/dictionaries'
if [ ! -e "${kodict}.oxt" ]; then
	curl -s -O "https://extensions.libreoffice.org/extensions/korean-spellchecker/0-6-0-1/@@download/file/${kodict}.oxt"
	echo ' DONE'
else
	echo ' SKIPPED'
fi

echo -n 'Downloading Indonesian dictionary...'
iddict='id_id'
idfiles='id_id'
if [ ! -e "${iddict}.oxt" ]; then
	curl -s -O "https://extensions.libreoffice.org/extensions/indonesian-dictionary-kamus-indonesia-by-benitius/2.0/@@download/file/${iddict}.oxt"
	echo ' DONE'
else
	echo ' SKIPPED'
fi

echo -n 'Downloading Turkish dictionary...'
trdict='oo-turkish-dict-v1-2'
trfiles="${trdict}/dictionaries"
if [ ! -e "${trdict}.oxt" ]; then
	curl -s -O "https://extensions.libreoffice.org/extensions/turkish-spellcheck-dictionary/1.2/@@download/file/${trdict}.oxt"
	echo ' DONE'
else
	echo ' SKIPPED'
fi


# Extract
echo -n 'Extracting LibreOffice dictionaries...'
if [ ! -e "libreoffice-${loversion}" ]; then
	tar -xaf "${lodict}.tar.xz"
	echo ' DONE'
else
	echo ' SKIPPED'
fi

echo -n 'Extracting Armenian dictionary...'
if [ ! -e "${hydict}" ]; then
	unzip -qq "${hydict}.oxt" -d "${hydict}"
	echo ' DONE'
else
	echo ' SKIPPED'
fi

echo -n 'Extracting Esperanto dictionary...'
if [ ! -e "${eodict}" ]; then
	unzip -qq "${eodict}.oxt" -d "${eodict}"
	echo ' DONE'
else
	echo ' SKIPPED'
fi

echo -n 'Extracting Finnish dictionary...'
if [ ! -e voikko ]; then
	unzip -qq voikko.oxt -d voikko
	echo ' DONE'
else
	echo ' SKIPPED'
fi

echo -n 'Extracting Korean dictionary...'
if [ ! -e "${kodict}" ]; then
	unzip -qq "${kodict}.oxt" -d "${kodict}"
	echo ' DONE'
else
	echo ' SKIPPED'
fi

echo -n 'Extracting Indonesian dictionary...'
if [ ! -e "${iddict}" ]; then
	unzip -qq "${iddict}.oxt" -d "${iddict}"
	echo ' DONE'
else
	echo ' SKIPPED'
fi

echo -n 'Extracting Turkish dictionary...'
if [ ! -e "${trdict}" ]; then
	unzip -qq "${trdict}.oxt" -d "${trdict}"
	echo ' DONE'
else
	echo ' SKIPPED'
fi


# Copy files
echo -n 'Copying...'
mkdir dicts
echo -n ' af_ZA'
cp -a $lofiles/af_ZA/af_ZA.aff dicts/af_ZA.aff
cp -a $lofiles/af_ZA/af_ZA.dic dicts/af_ZA.dic
echo -n ' ar'
cp -a $lofiles/ar/ar.aff dicts/ar.aff
cp -a $lofiles/ar/ar.dic dicts/ar.dic
echo -n ' ca'
cp -a $lofiles/ca/dictionaries/ca.aff dicts/ca.aff
cp -a $lofiles/ca/dictionaries/ca.dic dicts/ca.dic
echo -n ' cs'
cp -a $lofiles/cs_CZ/cs_CZ.aff dicts/cs.aff
cp -a $lofiles/cs_CZ/cs_CZ.dic dicts/cs.dic
echo -n ' da'
cp -a $lofiles/da_DK/da_DK.aff dicts/da.aff
cp -a $lofiles/da_DK/da_DK.dic dicts/da.dic
echo -n ' de_AT'
cp -a $lofiles/de/de_DE_frami.aff dicts/de_AT.aff
cp -a $lofiles/de/de_AT_frami.dic dicts/de_AT.dic
echo -n ' de_CH'
cp -a $lofiles/de/de_DE_frami.aff dicts/de_CH.aff
cp -a $lofiles/de/de_CH_frami.dic dicts/de_CH.dic
echo -n ' de_DE'
cp -a $lofiles/de/de_DE_frami.aff dicts/de_DE.aff
cp -a $lofiles/de/de_DE_frami.dic dicts/de_DE.dic
echo -n ' el'
cp -a $lofiles/el_GR/el_GR.aff dicts/el.aff
cp -a $lofiles/el_GR/el_GR.dic dicts/el.dic
echo -n ' en_AU'
cp -a $lofiles/en/en_AU.aff dicts/en_AU.aff
cp -a $lofiles/en/en_AU.dic dicts/en_AU.dic
echo -n ' en_CA'
cp -a $lofiles/en/en_CA.aff dicts/en_CA.aff
cp -a $lofiles/en/en_CA.dic dicts/en_CA.dic
echo -n ' en_GB'
cp -a $lofiles/en/en_GB.aff dicts/en_GB.aff
cp -a $lofiles/en/en_GB.dic dicts/en_GB.dic
echo -n ' en_US'
cp -a $lofiles/en/en_US.aff dicts/en_US.aff
cp -a $lofiles/en/en_US.dic dicts/en_US.dic
echo -n ' en_ZA'
cp -a $lofiles/en/en_ZA.aff dicts/en_ZA.aff
cp -a $lofiles/en/en_ZA.dic dicts/en_ZA.dic
echo -n ' eo'
cp -a $eofiles/eo_ilo.aff dicts/eo.aff
cp -a $eofiles/eo_ilo.dic dicts/eo.dic
echo -n ' es'
cp -a $lofiles/es/es_ANY.aff dicts/es.aff
cp -a $lofiles/es/es_ANY.dic dicts/es.dic
echo -n ' fi'
cp -a voikko/voikko/2 dicts
cp -a voikko/voikko/Windows-32bit-WindowsPE/libvoikko-1.dll dicts
echo -n ' fr'
cp -a $lofiles/fr_FR/fr.aff dicts/fr.aff
cp -a $lofiles/fr_FR/fr.dic dicts/fr.dic
echo -n ' he'
cp -a $lofiles/he_IL/he_IL.aff dicts/he.aff
cp -a $lofiles/he_IL/he_IL.dic dicts/he.dic
echo -n ' hu'
cp -a $lofiles/hu_HU/hu_HU.aff dicts/hu.aff
cp -a $lofiles/hu_HU/hu_HU.dic dicts/hu.dic
echo -n ' hy'
cp -a $hyfiles/hy_am_e_1940.aff dicts/hy.aff
cp -a $hyfiles/hy_am_e_1940.dic dicts/hy.dic
echo -n ' id'
cp -a $idfiles/id_ID.aff dicts/id.aff
cp -a $idfiles/id_ID.dic dicts/id.dic
echo -n ' it'
cp -a $lofiles/it_IT/it_IT.aff dicts/it.aff
cp -a $lofiles/it_IT/it_IT.dic dicts/it.dic
echo -n ' ko'
cp -a $kofiles/ko-KR.aff dicts/ko.aff
cp -a $kofiles/ko-KR.dic dicts/ko.dic
echo -n ' lt'
cp -a $lofiles/lt_LT/lt.aff dicts/lt.aff
cp -a $lofiles/lt_LT/lt.dic dicts/lt.dic
echo -n ' nl'
cp -a $lofiles/nl_NL/nl_NL.aff dicts/nl.aff
cp -a $lofiles/nl_NL/nl_NL.dic dicts/nl.dic
echo -n ' pl'
cp -a $lofiles/pl_PL/pl_PL.aff dicts/pl.aff
cp -a $lofiles/pl_PL/pl_PL.dic dicts/pl.dic
echo -n ' pt_BR'
cp -a $lofiles/pt_BR/pt_BR.aff dicts/pt_BR.aff
cp -a $lofiles/pt_BR/pt_BR.dic dicts/pt_BR.dic
echo -n ' pt_PT'
cp -a $lofiles/pt_PT/pt_PT.aff dicts/pt_PT.aff
cp -a $lofiles/pt_PT/pt_PT.dic dicts/pt_PT.dic
echo -n ' ro'
cp -a $lofiles/ro/ro_RO.aff dicts/ro.aff
cp -a $lofiles/ro/ro_RO.dic dicts/ro.dic
echo -n ' ru'
cp -a $lofiles/ru_RU/ru_RU.aff dicts/ru.aff
cp -a $lofiles/ru_RU/ru_RU.dic dicts/ru.dic
echo -n ' sk'
cp -a $lofiles/sk_SK/sk_SK.aff dicts/sk.aff
cp -a $lofiles/sk_SK/sk_SK.dic dicts/sk.dic
echo -n ' sl'
cp -a $lofiles/sl_SI/sl_SI.aff dicts/sl.aff
cp -a $lofiles/sl_SI/sl_SI.dic dicts/sl.dic
echo -n ' sr'
cp -a $lofiles/sr/sr.aff dicts/sr.aff
cp -a $lofiles/sr/sr.dic dicts/sr.dic
cp -a $lofiles/sr/sr-Latn.aff dicts/sr-Latn.aff
cp -a $lofiles/sr/sr-Latn.dic dicts/sr-Latn.dic
echo -n ' sv'
cp -a $lofiles/sv_SE/sv_SE.aff dicts/sv.aff
cp -a $lofiles/sv_SE/sv_SE.dic dicts/sv.dic
echo -n ' tr'
cp -a $trfiles/tr-TR.aff dicts/tr.aff
cp -a $trfiles/tr-TR.dic dicts/tr.dic
echo -n ' uk'
cp -a $lofiles/uk_UA/uk_UA.aff dicts/uk.aff
cp -a $lofiles/uk_UA/uk_UA.dic dicts/uk.dic
echo -n ' vi'
cp -a $lofiles/vi/vi_VN.aff dicts/vi.aff
cp -a $lofiles/vi/vi_VN.dic dicts/vi.dic
echo ' DONE'


# Finish and clean up
cd ..
mv temp/dicts .
