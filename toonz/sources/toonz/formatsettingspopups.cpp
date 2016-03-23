

#include "formatsettingspopups.h"

// Tnz6 includes
#include "tapp.h"
#include "dvwidgets.h"

// TnzLib includes
#include "toonz/tscenehandle.h"
#include "toonz/sceneproperties.h"
#include "toonz/toonzscene.h"
#include "toonz/tcamera.h"
#include "toutputproperties.h"

#ifdef WIN32
#include "avicodecrestrictions.h"
#endif;

// TnzCore includes
#include "tlevel_io.h"
#include "tproperty.h"
#include "movsettings.h"
#include "timageinfo.h"

// Qt includes
#include <QComboBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMainWindow>

//**********************************************************************************
//    FormatSettingsPopup  implementation
//**********************************************************************************

FormatSettingsPopup::FormatSettingsPopup(
	QWidget *parent, const std::string &format, TPropertyGroup *props)
	: Dialog(parent), m_format(format), m_props(props), m_levelPath(TFilePath())
#ifdef WIN32
	  ,
	  m_codecRestriction(0), m_codecComboBox(0), m_configureCodec(0)
#endif
{
	setWindowTitle(tr("File Settings"));

	setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);

	m_mainLayout = new QGridLayout();
	m_mainLayout->setMargin(0);
	m_mainLayout->setVerticalSpacing(5);
	m_mainLayout->setHorizontalSpacing(5);
	m_mainLayout->setColumnStretch(0, 0);
	m_mainLayout->setColumnStretch(1, 1);

	int i = 0;
	for (i = 0; i < m_props->getPropertyCount(); i++) {
		if (dynamic_cast<TEnumProperty *>(m_props->getProperty(i)))
			buildPropertyComboBox(i, m_props);
		else if (dynamic_cast<TIntProperty *>(m_props->getProperty(i)))
			buildValueField(i, m_props);
		else if (dynamic_cast<TBoolProperty *>(m_props->getProperty(i)))
			buildPropertyCheckBox(i, m_props);
		else if (dynamic_cast<TStringProperty *>(m_props->getProperty(i)))
			buildPropertyLineEdit(i, m_props);
		else
			assert(false);
	}

#ifdef WIN32
	if (format == "avi") {
		m_codecRestriction = new QLabel(this);
		m_codecRestriction->setMinimumHeight(70);
		m_codecRestriction->setStyleSheet("border: 1px solid rgb(200,200,200);");
		m_mainLayout->addWidget(m_codecRestriction, m_mainLayout->rowCount(), 0, 1, 2);
		m_configureCodec = new QPushButton("Configure Codec", this);
		m_configureCodec->setFixedSize(100, WidgetHeight);
		m_mainLayout->addWidget(m_configureCodec, m_mainLayout->rowCount(), 0, 1, 2);
		connect(m_configureCodec, SIGNAL(released()), this, SLOT(onAviCodecConfigure()));
	}
#endif;

	m_topLayout->addLayout(m_mainLayout, 1);
}

//-----------------------------------------------------------------------------

void FormatSettingsPopup::setFormatProperties(TPropertyGroup *props)
{
	if (props == m_props)
		return;

	m_props = props;

	for (int i = 0; i < m_props->getPropertyCount(); i++) {
		if (TProperty *prop = m_props->getProperty(i))
			m_widgets[prop->getName()]->setProperty(prop);
	}
}

//-----------------------------------------------------------------------------

void FormatSettingsPopup::buildPropertyComboBox(int index, TPropertyGroup *props)
{
	TEnumProperty *prop = (TEnumProperty *)(props->getProperty(index));
	assert(prop);

	PropertyComboBox *comboBox = new PropertyComboBox(this, prop);
	m_widgets[prop->getName()] = comboBox;
	connect(comboBox, SIGNAL(currentIndexChanged(const QString)), this, SLOT(onComboBoxIndexChanged(const QString)));
	TEnumProperty::Range range = prop->getRange();
	int currIndex = -1;
	wstring defaultVal = prop->getValue();

	for (int i = 0; i < (int)range.size(); i++) {
		wstring nameProp = range[i];

		if (nameProp.find(L"16(GREYTONES)") != -1) //pezza per il tif: il 16 lo scrive male, e il 48 lo legge male...
			continue;
		/* if (nameProp.find(L"ThunderScan")!=-1) //pezza epr il tif, molte compressioni non vanno..scive male il file
      break;*/

		if (nameProp == defaultVal)
			currIndex = comboBox->count();
		comboBox->addItem(QString::fromStdWString(nameProp));
	}
	if (currIndex >= 0)
		comboBox->setCurrentIndex(currIndex);

	int row = m_mainLayout->rowCount();
	m_mainLayout->addWidget(new QLabel(tr(prop->getName().c_str()) + ":", this), row, 0, Qt::AlignRight | Qt::AlignVCenter);
	m_mainLayout->addWidget(comboBox, row, 1);

#ifdef WIN32
	if (m_format == "avi")
		m_codecComboBox = comboBox;
#endif;
}

//-----------------------------------------------------------------------------

void FormatSettingsPopup::buildValueField(int index, TPropertyGroup *props)
{
	TIntProperty *prop = (TIntProperty *)(props->getProperty(index));
	assert(prop);

	PropertyIntField *v = new PropertyIntField(this, prop);
	m_widgets[prop->getName()] = v;

	int row = m_mainLayout->rowCount();
	m_mainLayout->addWidget(new QLabel(tr(prop->getName().c_str()) + ":", this), row, 0, Qt::AlignRight | Qt::AlignVCenter);
	m_mainLayout->addWidget(v, row, 1);

	v->setValues(prop->getValue(), prop->getRange().first, prop->getRange().second);
}

//-----------------------------------------------------------------------------

void FormatSettingsPopup::buildPropertyCheckBox(int index, TPropertyGroup *props)
{
	TBoolProperty *prop = (TBoolProperty *)(props->getProperty(index));
	assert(prop);

	PropertyCheckBox *v = new PropertyCheckBox(tr(prop->getName().c_str()), this, prop);
	m_widgets[prop->getName()] = v;

	m_mainLayout->addWidget(v, m_mainLayout->rowCount(), 1);

	v->setChecked(prop->getValue());
}

//-----------------------------------------------------------------------------

void FormatSettingsPopup::buildPropertyLineEdit(int index, TPropertyGroup *props)
{
	TStringProperty *prop = (TStringProperty *)(props->getProperty(index));
	assert(prop);

	PropertyLineEdit *lineEdit = new PropertyLineEdit(this, prop);
	m_widgets[prop->getName()] = lineEdit;
	lineEdit->setText(tr(toString(prop->getValue()).c_str()));

	int row = m_mainLayout->rowCount();
	m_mainLayout->addWidget(new QLabel(tr(prop->getName().c_str()) + ":", this), row, 0, Qt::AlignRight | Qt::AlignVCenter);
	m_mainLayout->addWidget(lineEdit, row, 1);
}

//-----------------------------------------------------------------------------

#ifdef WIN32

void FormatSettingsPopup::onComboBoxIndexChanged(const QString codecName)
{
	if (!m_codecRestriction)
		return;
	QString msg;
	AviCodecRestrictions::getRestrictions(codecName.toStdWString(), msg);
	m_codecRestriction->setText(msg);
}

//-----------------------------------------------------------------------------

void FormatSettingsPopup::onAviCodecConfigure()
{
	QString codecName = m_codecComboBox->currentText();
	wstring wCodecName = codecName.toStdWString();
	if (AviCodecRestrictions::canBeConfigured(wCodecName))
		AviCodecRestrictions::openConfiguration(wCodecName, (HWND)winId());
}

#endif

//-----------------------------------------------------------------------------

void FormatSettingsPopup::showEvent(QShowEvent *se)
{
#ifdef WIN32
	if (m_format == "avi") {
		assert(m_codecComboBox);
		m_codecComboBox->blockSignals(true);
		m_codecComboBox->clear();
		ToonzScene *scene = TApp::instance()->getCurrentScene()->getScene();
		TEnumProperty *eProps = dynamic_cast<TEnumProperty *>(m_props->getProperty(0));
		assert(eProps);

		TDimension res(0, 0);
		if (m_levelPath.isEmpty())
			res = scene->getCurrentCamera()->getRes();
		else {
			TLevelReaderP lr(m_levelPath);
			TLevelP level = lr->loadInfo();
			const TImageInfo *info = lr->getImageInfo(level->begin()->first);
			res.lx = info->m_lx;
			res.ly = info->m_ly;
		}

		TEnumProperty::Range range = eProps->getRange();
		int currIndex = -1;
		wstring defaultVal = eProps->getValue();

		QMap<wstring, bool> usableCodecs = AviCodecRestrictions::getUsableCodecs(res);
		for (int i = 0; i < (int)range.size(); i++) {
			wstring nameProp = range[i];
			if (nameProp == L"Uncompressed" || (usableCodecs.contains(nameProp) && usableCodecs[nameProp])) {
				if (nameProp == defaultVal)
					currIndex = m_codecComboBox->count();
				m_codecComboBox->addItem(QString::fromStdWString(nameProp));
			}
		}
		m_codecComboBox->blockSignals(false);
		if (currIndex >= 0)
			m_codecComboBox->setCurrentIndex(currIndex);
	}

#endif

	Dialog::showEvent(se);
}

//**********************************************************************************
//    API  functions
//**********************************************************************************

FormatSettingsPopup *openFormatSettingsPopup(
	QWidget *parent, const std::string &format, TPropertyGroup *props, const TFilePath &levelPath)
{
	if (format == "mov" || format == "3gp") // trattato diversamente; il format popup dei mov e' quello di quicktime
	{
		// gmt 26/9/07. con la penna capita spesso di premere ripetutamente il bottone.
		// si aprono due popup uno sopra l'altro (brutto di per se e puo' causare dei crash misteriosi)
		static bool popupIsOpen = false;
		if (popupIsOpen)
			return 0;

		popupIsOpen = true;
		openMovSettingsPopup(props);
		popupIsOpen = false;

		return 0;
	}

	if (!props || props->getPropertyCount() == 0)
		return 0;

	FormatSettingsPopup *popup = new FormatSettingsPopup(parent, format, props);
	popup->setAttribute(Qt::WA_DeleteOnClose);
	popup->setModal(true);

	if (!levelPath.isEmpty())
		popup->setLevelPath(levelPath);

	popup->show();
	popup->raise();

	return popup;
}