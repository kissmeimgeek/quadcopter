import json
import sys
from PyQt5.QtWidgets import QApplication, QWidget, QLineEdit, QMainWindow, QTextEdit, QVBoxLayout, QHBoxLayout, QSlider, QLabel, QPushButton, QCheckBox, QComboBox, QGroupBox, QScrollArea
from PyQt5.QtGui import QFont
from PyQt5.QtCore import Qt, pyqtSignal, QRect

class FloatSlider(QSlider):

    # create our our signal that we can connect to if necessary
    floatValueChanged = pyqtSignal(float)

    def __init__(self, decimals=3):
        super(FloatSlider, self).__init__(Qt.Horizontal)
        self._multi = 10 ** decimals
        self.valueChanged.connect(self.emitFloatValueChanged)

    def emitFloatValueChanged(self):
        value = float(super(FloatSlider, self).value())/self._multi
        self.floatValueChanged.emit(value)

    def value(self):
        return float(super(FloatSlider, self).value()) / self._multi

    def setMinimum(self, value):
        return super(FloatSlider, self).setMinimum(value * self._multi)

    def setMaximum(self, value):
        return super(FloatSlider, self).setMaximum(value * self._multi)

    def setSingleStep(self, value):
        return super(FloatSlider, self).setSingleStep(value * self._multi)

    def singleStep(self):
        return float(super(FloatSlider, self).singleStep()) / self._multi

    def setValue(self, value):
        super(FloatSlider, self).setValue(int(value * self._multi))


class DataRessourceWidget(QWidget):

    def __init__(self, value_widget, read_callback, key, permissions):
        super().__init__()
        self.read_callback = read_callback
        self.value_widget = value_widget
        self.key = key
        self.label = QLabel(key.split('.')[-1])
        self.button = QPushButton('read')
        self.button.clicked.connect(self.on_button_clicked)
        if not 'read' in permissions:
            self.button.setEnabled(False)
        if not 'write' in permissions:
            self.value_widget.setEnabled(False)
        self.layout = QHBoxLayout()
        self.layout.addWidget(self.label)
        self.layout.addWidget(value_widget)
        self.layout.addWidget(self.button)
        self.setLayout(self.layout)

    def on_button_clicked(self):
        self.read_callback(self.key)


class IntegerRessourceWidget(DataRessourceWidget):

    def __init__(self, write_callback, read_callback, key, permissions, unit, minimum=0, maximum=10):
        super().__init__(QSlider(Qt.Horizontal), read_callback, key, permissions)
        self.write_callback = write_callback
        self.unit = unit
        self.value_widget.setMinimum(minimum)
        self.value_widget.setMaximum(maximum)
        self.value_widget.valueChanged.connect(self.on_value_changed)

    def on_value_changed(self, value):
        self.write_callback(self.key, value)


class FloatRessourceWidget(DataRessourceWidget):

    def __init__(self, write_callback, read_callback, key, permissions, unit, minimum=0, maximum=10):
        super().__init__(FloatSlider(), read_callback, key, permissions)
        self.write_callback = write_callback
        self.unit = unit
        self.value_widget.setMinimum(minimum)
        self.value_widget.setMaximum(maximum)
        self.value_widget.floatValueChanged.connect(self.on_value_changed)

    def on_value_changed(self, value):
        self.write_callback(self.key, value)


class BoolRessourceWidget(DataRessourceWidget):

    def __init__(self, write_callback, read_callback, key, permissions):
        super().__init__(QCheckBox(), read_callback, key, permissions)
        self.write_callback = write_callback
        self.value_widget.stateChanged.connect(self.on_value_changed)

    def on_value_changed(self, value):
        self.write_callback(self.key, value)


class EnumRessourceWidget(DataRessourceWidget):

    def __init__(self, write_callback, read_callback, key, permissions, values):
        super().__init__(QComboBox(), read_callback, key, permissions)
        self.write_callback = write_callback
        self.values = values
        self.value_widget.addItems(values.keys())
        self.value_widget.currentIndexChanged.connect(self.on_value_changed)

    def on_value_changed(self, index):
        self.write_callback(self.key, self.value_widget.currentText())


class MainWindow(QMainWindow):

    def __init__(self, data_model):
        super().__init__()
        self.data_model = data_model
        self.widgets = {}
        self.setup_ui()

    def load_model(self, data_model, container_layout, current_key = ''):
        for key, content in data_model.items():
            new_key = current_key + key
            if isinstance(content, dict):
                if 'type' in content:
                    if content['type'] == 'integer':
                        widget = IntegerRessourceWidget(self.on_write_request, self.on_read_request, new_key, content['permissions'], 
                                                        content['unit'], content['min'], content['max'])
                    elif content['type'] == 'float':
                        widget = FloatRessourceWidget(self.on_write_request, self.on_read_request, new_key, content['permissions'], 
                                                    content['unit'], 0.0, 1.0)
                    elif content['type'] == 'bool':
                        widget = BoolRessourceWidget(self.on_write_request, self.on_read_request, new_key, content['permissions'])
                    elif content['type'] == 'enum':
                        widget = EnumRessourceWidget(self.on_write_request, self.on_read_request, new_key, content['permissions'],
                                                    content['values'])
                    else:
                        raise Exception("Invalid ressource type")
                    self.widgets[new_key] = widget
                    container_layout.addWidget(widget)
                else:
                    vbox = QVBoxLayout()
                    self.load_model(data_model[key], vbox, new_key + '.')
                    widget = QGroupBox(key)
                    widget.setLayout(vbox)
                    container_layout.addWidget(widget)

    def setup_ui(self):
        self.centralWidget = QWidget()
        layout = QVBoxLayout(self.centralWidget)

        self.scrollArea = QScrollArea(self.centralWidget)
        layout.addWidget(self.scrollArea)

        self.scrollAreaWidgetContents = QWidget()
        self.scrollAreaWidgetContents.setGeometry(QRect(0, 0, 970, 3200))
        self.scrollArea.setWidget(self.scrollAreaWidgetContents)

        layout = QVBoxLayout(self.scrollAreaWidgetContents)     
        self.setCentralWidget(self.centralWidget)

        self.centralWidget.setStyleSheet("QGroupBox { padding-top: 20px; font-size:14px; text-transform: uppercase; background-color: rgb(230, 230, 230); border: 1px solid rgb(100, 100, 100); }")

        self.setWindowTitle('GUI')

        self.load_model(data_model, layout)

        self.setCentralWidget(self.centralWidget)

        self.resize(1000, 800)
        self.show()

    def on_write_request(self, key, value):
        print(f'write {key} {value}')
    
    def on_read_request(self, key):
        print(f'read {key}')

with open('data_model.json') as json_file:
        data_model = json.load(json_file)

app = QApplication(sys.argv)
w = MainWindow(data_model)
sys.exit(app.exec_())