# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'pulseParamGUI.ui'
#
# Created: Sun Jan 22 20:29:07 2012
#      by: pyside-uic 0.2.13 running on PySide 1.1.0
#
# WARNING! All changes made in this file will be lost!

from PySide import QtCore, QtGui

class Ui_Dialog(object):
    def setupUi(self, Dialog):
        Dialog.setObjectName("Dialog")
        Dialog.resize(510, 371)
        self.verticalLayout_3 = QtGui.QVBoxLayout(Dialog)
        self.verticalLayout_3.setObjectName("verticalLayout_3")
        self.label_14 = QtGui.QLabel(Dialog)
        font = QtGui.QFont()
        font.setWeight(75)
        font.setBold(True)
        self.label_14.setFont(font)
        self.label_14.setObjectName("label_14")
        self.verticalLayout_3.addWidget(self.label_14)
        self.horizontalLayout_5 = QtGui.QHBoxLayout()
        self.horizontalLayout_5.setObjectName("horizontalLayout_5")
        self.groupBox = QtGui.QGroupBox(Dialog)
        self.groupBox.setObjectName("groupBox")
        self.verticalLayout = QtGui.QVBoxLayout(self.groupBox)
        self.verticalLayout.setObjectName("verticalLayout")
        self.formLayout = QtGui.QFormLayout()
        self.formLayout.setFieldGrowthPolicy(QtGui.QFormLayout.FieldsStayAtSizeHint)
        self.formLayout.setObjectName("formLayout")
        self.label = QtGui.QLabel(self.groupBox)
        font = QtGui.QFont()
        font.setWeight(75)
        font.setBold(True)
        self.label.setFont(font)
        self.label.setObjectName("label")
        self.formLayout.setWidget(0, QtGui.QFormLayout.LabelRole, self.label)
        self.qubitComboBox = QtGui.QComboBox(self.groupBox)
        self.qubitComboBox.setObjectName("qubitComboBox")
        self.qubitComboBox.addItem("")
        self.qubitComboBox.addItem("")
        self.qubitComboBox.addItem("")
        self.formLayout.setWidget(0, QtGui.QFormLayout.FieldRole, self.qubitComboBox)
        self.label_2 = QtGui.QLabel(self.groupBox)
        self.label_2.setObjectName("label_2")
        self.formLayout.setWidget(1, QtGui.QFormLayout.LabelRole, self.label_2)
        self.piAmp = QtGui.QLineEdit(self.groupBox)
        self.piAmp.setObjectName("piAmp")
        self.formLayout.setWidget(1, QtGui.QFormLayout.FieldRole, self.piAmp)
        self.label_3 = QtGui.QLabel(self.groupBox)
        self.label_3.setObjectName("label_3")
        self.formLayout.setWidget(2, QtGui.QFormLayout.LabelRole, self.label_3)
        self.pi2Amp = QtGui.QLineEdit(self.groupBox)
        self.pi2Amp.setObjectName("pi2Amp")
        self.formLayout.setWidget(2, QtGui.QFormLayout.FieldRole, self.pi2Amp)
        self.label_4 = QtGui.QLabel(self.groupBox)
        self.label_4.setObjectName("label_4")
        self.formLayout.setWidget(3, QtGui.QFormLayout.LabelRole, self.label_4)
        self.delta = QtGui.QLineEdit(self.groupBox)
        self.delta.setObjectName("delta")
        self.formLayout.setWidget(3, QtGui.QFormLayout.FieldRole, self.delta)
        self.label_5 = QtGui.QLabel(self.groupBox)
        self.label_5.setObjectName("label_5")
        self.formLayout.setWidget(4, QtGui.QFormLayout.LabelRole, self.label_5)
        self.sigma = QtGui.QLineEdit(self.groupBox)
        self.sigma.setObjectName("sigma")
        self.formLayout.setWidget(4, QtGui.QFormLayout.FieldRole, self.sigma)
        self.label_6 = QtGui.QLabel(self.groupBox)
        self.label_6.setObjectName("label_6")
        self.formLayout.setWidget(5, QtGui.QFormLayout.LabelRole, self.label_6)
        self.pulseLength = QtGui.QLineEdit(self.groupBox)
        self.pulseLength.setObjectName("pulseLength")
        self.formLayout.setWidget(5, QtGui.QFormLayout.FieldRole, self.pulseLength)
        self.label_7 = QtGui.QLabel(self.groupBox)
        self.label_7.setObjectName("label_7")
        self.formLayout.setWidget(6, QtGui.QFormLayout.LabelRole, self.label_7)
        self.pulseType = QtGui.QLineEdit(self.groupBox)
        self.pulseType.setObjectName("pulseType")
        self.formLayout.setWidget(6, QtGui.QFormLayout.FieldRole, self.pulseType)
        self.verticalLayout.addLayout(self.formLayout)
        self.horizontalLayout_5.addWidget(self.groupBox)
        self.groupBox_2 = QtGui.QGroupBox(Dialog)
        self.groupBox_2.setObjectName("groupBox_2")
        self.verticalLayout_2 = QtGui.QVBoxLayout(self.groupBox_2)
        self.verticalLayout_2.setObjectName("verticalLayout_2")
        self.formLayout_2 = QtGui.QFormLayout()
        self.formLayout_2.setFieldGrowthPolicy(QtGui.QFormLayout.FieldsStayAtSizeHint)
        self.formLayout_2.setObjectName("formLayout_2")
        self.label_9 = QtGui.QLabel(self.groupBox_2)
        self.label_9.setObjectName("label_9")
        self.formLayout_2.setWidget(1, QtGui.QFormLayout.LabelRole, self.label_9)
        self.bufferPadding = QtGui.QLineEdit(self.groupBox_2)
        self.bufferPadding.setObjectName("bufferPadding")
        self.formLayout_2.setWidget(1, QtGui.QFormLayout.FieldRole, self.bufferPadding)
        self.label_10 = QtGui.QLabel(self.groupBox_2)
        self.label_10.setObjectName("label_10")
        self.formLayout_2.setWidget(2, QtGui.QFormLayout.LabelRole, self.label_10)
        self.bufferReset = QtGui.QLineEdit(self.groupBox_2)
        self.bufferReset.setObjectName("bufferReset")
        self.formLayout_2.setWidget(2, QtGui.QFormLayout.FieldRole, self.bufferReset)
        self.label_11 = QtGui.QLabel(self.groupBox_2)
        self.label_11.setObjectName("label_11")
        self.formLayout_2.setWidget(3, QtGui.QFormLayout.LabelRole, self.label_11)
        self.bufferDelay = QtGui.QLineEdit(self.groupBox_2)
        self.bufferDelay.setObjectName("bufferDelay")
        self.formLayout_2.setWidget(3, QtGui.QFormLayout.FieldRole, self.bufferDelay)
        self.label_12 = QtGui.QLabel(self.groupBox_2)
        self.label_12.setObjectName("label_12")
        self.formLayout_2.setWidget(4, QtGui.QFormLayout.LabelRole, self.label_12)
        self.offset = QtGui.QLineEdit(self.groupBox_2)
        self.offset.setObjectName("offset")
        self.formLayout_2.setWidget(4, QtGui.QFormLayout.FieldRole, self.offset)
        self.label_13 = QtGui.QLabel(self.groupBox_2)
        self.label_13.setObjectName("label_13")
        self.formLayout_2.setWidget(5, QtGui.QFormLayout.LabelRole, self.label_13)
        self.delay = QtGui.QLineEdit(self.groupBox_2)
        self.delay.setObjectName("delay")
        self.formLayout_2.setWidget(5, QtGui.QFormLayout.FieldRole, self.delay)
        self.label_15 = QtGui.QLabel(self.groupBox_2)
        self.label_15.setObjectName("label_15")
        self.formLayout_2.setWidget(6, QtGui.QFormLayout.LabelRole, self.label_15)
        self.T = QtGui.QLineEdit(self.groupBox_2)
        self.T.setObjectName("T")
        self.formLayout_2.setWidget(6, QtGui.QFormLayout.FieldRole, self.T)
        self.label_8 = QtGui.QLabel(self.groupBox_2)
        font = QtGui.QFont()
        font.setWeight(75)
        font.setBold(True)
        self.label_8.setFont(font)
        self.label_8.setObjectName("label_8")
        self.formLayout_2.setWidget(0, QtGui.QFormLayout.LabelRole, self.label_8)
        self.channelComboBox = QtGui.QComboBox(self.groupBox_2)
        self.channelComboBox.setObjectName("channelComboBox")
        self.channelComboBox.addItem("")
        self.channelComboBox.addItem("")
        self.channelComboBox.addItem("")
        self.channelComboBox.addItem("")
        self.formLayout_2.setWidget(0, QtGui.QFormLayout.FieldRole, self.channelComboBox)
        self.verticalLayout_2.addLayout(self.formLayout_2)
        self.horizontalLayout_5.addWidget(self.groupBox_2)
        self.verticalLayout_3.addLayout(self.horizontalLayout_5)
        self.horizontalLayout_4 = QtGui.QHBoxLayout()
        self.horizontalLayout_4.setObjectName("horizontalLayout_4")
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_4.addItem(spacerItem)
        self.refreshButton = QtGui.QPushButton(Dialog)
        self.refreshButton.setObjectName("refreshButton")
        self.horizontalLayout_4.addWidget(self.refreshButton)
        self.updateButton = QtGui.QPushButton(Dialog)
        self.updateButton.setObjectName("updateButton")
        self.horizontalLayout_4.addWidget(self.updateButton)
        self.verticalLayout_3.addLayout(self.horizontalLayout_4)
        spacerItem1 = QtGui.QSpacerItem(20, 12, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.verticalLayout_3.addItem(spacerItem1)
        self.label.setBuddy(self.qubitComboBox)
        self.label_2.setBuddy(self.piAmp)
        self.label_3.setBuddy(self.pi2Amp)
        self.label_4.setBuddy(self.delta)
        self.label_5.setBuddy(self.sigma)
        self.label_6.setBuddy(self.pulseLength)
        self.label_7.setBuddy(self.pulseType)
        self.label_9.setBuddy(self.bufferPadding)
        self.label_10.setBuddy(self.bufferReset)
        self.label_11.setBuddy(self.bufferDelay)
        self.label_12.setBuddy(self.offset)
        self.label_13.setBuddy(self.delay)
        self.label_15.setBuddy(self.T)
        self.label_8.setBuddy(self.channelComboBox)

        self.retranslateUi(Dialog)
        QtCore.QMetaObject.connectSlotsByName(Dialog)
        Dialog.setTabOrder(self.updateButton, self.refreshButton)
        Dialog.setTabOrder(self.refreshButton, self.piAmp)
        Dialog.setTabOrder(self.piAmp, self.pi2Amp)
        Dialog.setTabOrder(self.pi2Amp, self.delta)
        Dialog.setTabOrder(self.delta, self.sigma)
        Dialog.setTabOrder(self.sigma, self.pulseLength)
        Dialog.setTabOrder(self.pulseLength, self.pulseType)
        Dialog.setTabOrder(self.pulseType, self.bufferPadding)
        Dialog.setTabOrder(self.bufferPadding, self.bufferReset)
        Dialog.setTabOrder(self.bufferReset, self.bufferDelay)
        Dialog.setTabOrder(self.bufferDelay, self.offset)
        Dialog.setTabOrder(self.offset, self.delay)
        Dialog.setTabOrder(self.delay, self.T)
        Dialog.setTabOrder(self.T, self.qubitComboBox)
        Dialog.setTabOrder(self.qubitComboBox, self.channelComboBox)

    def retranslateUi(self, Dialog):
        Dialog.setWindowTitle(QtGui.QApplication.translate("Dialog", "Dialog", None, QtGui.QApplication.UnicodeUTF8))
        self.label_14.setText(QtGui.QApplication.translate("Dialog", "Pulse Parameters", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("Dialog", "Qubit", None, QtGui.QApplication.UnicodeUTF8))
        self.qubitComboBox.setItemText(0, QtGui.QApplication.translate("Dialog", "q1", None, QtGui.QApplication.UnicodeUTF8))
        self.qubitComboBox.setItemText(1, QtGui.QApplication.translate("Dialog", "q2", None, QtGui.QApplication.UnicodeUTF8))
        self.qubitComboBox.setItemText(2, QtGui.QApplication.translate("Dialog", "q1q2", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("Dialog", "piAmp", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("Dialog", "pi2Amp", None, QtGui.QApplication.UnicodeUTF8))
        self.label_4.setText(QtGui.QApplication.translate("Dialog", "delta", None, QtGui.QApplication.UnicodeUTF8))
        self.label_5.setText(QtGui.QApplication.translate("Dialog", "sigma", None, QtGui.QApplication.UnicodeUTF8))
        self.label_6.setText(QtGui.QApplication.translate("Dialog", "pulseLength", None, QtGui.QApplication.UnicodeUTF8))
        self.label_7.setText(QtGui.QApplication.translate("Dialog", "pulseType", None, QtGui.QApplication.UnicodeUTF8))
        self.label_9.setText(QtGui.QApplication.translate("Dialog", "bufferPadding", None, QtGui.QApplication.UnicodeUTF8))
        self.label_10.setText(QtGui.QApplication.translate("Dialog", "bufferReset", None, QtGui.QApplication.UnicodeUTF8))
        self.label_11.setText(QtGui.QApplication.translate("Dialog", "bufferDelay", None, QtGui.QApplication.UnicodeUTF8))
        self.label_12.setText(QtGui.QApplication.translate("Dialog", "offset", None, QtGui.QApplication.UnicodeUTF8))
        self.label_13.setText(QtGui.QApplication.translate("Dialog", "delay", None, QtGui.QApplication.UnicodeUTF8))
        self.label_15.setText(QtGui.QApplication.translate("Dialog", "T", None, QtGui.QApplication.UnicodeUTF8))
        self.label_8.setText(QtGui.QApplication.translate("Dialog", "Channel", None, QtGui.QApplication.UnicodeUTF8))
        self.channelComboBox.setItemText(0, QtGui.QApplication.translate("Dialog", "Tek12", None, QtGui.QApplication.UnicodeUTF8))
        self.channelComboBox.setItemText(1, QtGui.QApplication.translate("Dialog", "Tek34", None, QtGui.QApplication.UnicodeUTF8))
        self.channelComboBox.setItemText(2, QtGui.QApplication.translate("Dialog", "BBN12", None, QtGui.QApplication.UnicodeUTF8))
        self.channelComboBox.setItemText(3, QtGui.QApplication.translate("Dialog", "BBN34", None, QtGui.QApplication.UnicodeUTF8))
        self.refreshButton.setText(QtGui.QApplication.translate("Dialog", "Refresh", None, QtGui.QApplication.UnicodeUTF8))
        self.updateButton.setText(QtGui.QApplication.translate("Dialog", "Update", None, QtGui.QApplication.UnicodeUTF8))
