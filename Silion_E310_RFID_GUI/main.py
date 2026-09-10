import sys,csv,time
from datetime import datetime
from PySide6.QtCore import Qt,QTimer,QSettings
from PySide6.QtGui import QAction,QFont
from PySide6.QtWidgets import QApplication,QMainWindow,QWidget,QVBoxLayout,QHBoxLayout,QGridLayout,QFormLayout,QFrame,QLabel,QPushButton,QComboBox,QLineEdit,QSpinBox,QDoubleSpinBox,QCheckBox,QTableWidget,QTableWidgetItem,QPlainTextEdit,QSplitter,QTabWidget,QFileDialog,QMessageBox,QHeaderView,QStatusBar,QScrollArea
from serial_manager import SerialManager
from protocol import parse_line
import theme
from ui_builder import build_ui

INFO=["GET_VERSION","GET_SERIAL","GET_TEMPERATURE","GET_REGION","GET_ANTENNA","GET_POWER","GET_PROTOCOL","GET_SESSION","GET_FREQUENCY"]

class Main(QMainWindow):
    def __init__(self):
        super().__init__(); self.setWindowTitle("RFID Reader"); self.resize(1500,920); self.setMinimumSize(1200,760)
        self.settings=QSettings("SilionRFID","E310Reader"); self.dark=self.settings.value("dark",True,type=bool)
        self.connected=False; self.inventory=False; self.total=0; self.epcs={}; self.selected_epc=''; self.epc_write_pending=''; self.last_read_data=''; self.poll_mode='MULTI'; self.single_poll_timeout_ms=3000; self.single_poll_started=False; self.tag_filter=''; self.filter_exact=False; self.first_tag=None; self.pending=None; self.queue=[]; self.backlog=[]; self.pump_scheduled=False; self.info_scheduled=False
        self.io=SerialManager()
        self.io.line_signal.connect(self.on_line)
        self.io.raw_signal.connect(self.on_raw)
        self.io.state_signal.connect(self.on_state)
        self.io.error_signal.connect(self.on_error)
        self.io.tx_signal.connect(self.on_tx)
        build_ui(self); self.restore(); self.apply_theme(); self.refresh_ports(); self.log("Application ready. Select the STM32 ST-LINK VCP port.")
        self.port_timer=QTimer(self); self.port_timer.timeout.connect(self.refresh_ports); self.port_timer.start(2500)
        self.status_timer=QTimer(self); self.status_timer.timeout.connect(self.status_poll); self.status_timer.start(15000)
        self.timeout_timer=QTimer(self); self.timeout_timer.timeout.connect(self.check_timeout); self.timeout_timer.start(100)

    def card(self): f=QFrame(); f.setObjectName("Card"); return f
    def sec(self,t):
        x=QLabel(t); x.setObjectName("Section"); x.setWordWrap(True); x.setMinimumWidth(0); x.setToolTip(t); return x

    def build(self):
        root=QWidget(); outer=QVBoxLayout(root); outer.setContentsMargins(12,10,12,8); outer.setSpacing(10)
        top=self.card(); g=QGridLayout(top); g.setContentsMargins(14,11,14,11)
        vb=QVBoxLayout(); title=QLabel("Silion / Impinj E310 RFID Reader"); title.setObjectName("Title"); title.setToolTip("Silion / Impinj E310 RFID Reader"); sub=QLabel("STM32 host interface  •  reliable serialized transactions"); sub.setObjectName("Sub"); vb.addWidget(title); vb.addWidget(sub); g.addLayout(vb,0,0,2,1)
        g.addWidget(QLabel("COM Port"),0,1); self.port=QComboBox(); self.port.setMinimumWidth(300); g.addWidget(self.port,0,2)
        g.addWidget(QLabel("Baud"),1,1); self.baud=QComboBox(); [self.baud.addItem(str(x),x) for x in (115200,57600,38400,19200,9600)]; g.addWidget(self.baud,1,2)
        self.connect=QPushButton("Connect"); self.connect.setObjectName("Accent"); self.connect.clicked.connect(self.toggle); g.addWidget(self.connect,0,3,2,1)
        self.conn=QLabel("● Disconnected"); self.conn.setStyleSheet("color:#C34C56;font-weight:700"); g.addWidget(self.conn,0,4,2,1)
        self.theme_btn=QPushButton(); self.theme_btn.clicked.connect(lambda:self.set_theme(not self.dark)); g.addWidget(self.theme_btn,0,5,2,1); g.setColumnStretch(6,1); outer.addWidget(top)

        kp=QHBoxLayout(); self.kv=[]
        for lab,val in [("Reader","Offline"),("Inventory","IDLE"),("Total Reads","0"),("Unique EPCs","0"),("Read Rate","0.0/s"),("Temperature","--")]:
            f=self.card(); l=QVBoxLayout(f); v=QLabel(val); v.setObjectName("Value"); c=QLabel(lab); c.setObjectName("Caption"); l.addWidget(v); l.addWidget(c); kp.addWidget(f); self.kv.append(v)
        outer.addLayout(kp)

        split=QSplitter(Qt.Horizontal); split.setChildrenCollapsible(False); scroll=QScrollArea(); scroll.setWidgetResizable(True); scroll.setFrameShape(QFrame.NoFrame)
        side=QWidget(); sl=QVBoxLayout(side); sl.setContentsMargins(0,0,6,0)
        ctl=self.card(); c=QVBoxLayout(ctl); c.addWidget(self.sec("Reader Control")); br=QGridLayout()
        self.single=QPushButton("◉  Single Poll"); self.single.setObjectName("Primary")
        self.multi=QPushButton("▶  Multi Poll"); self.multi.setObjectName("Primary")
        self.stop=QPushButton("■  Stop"); self.stop.setObjectName("Danger")
        self.resume=QPushButton("↻  Resume Multi")
        self.single.clicked.connect(self.single_poll); self.multi.clicked.connect(self.multi_poll)
        self.stop.clicked.connect(self.stop_inv); self.resume.clicked.connect(self.multi_poll)
        br.addWidget(self.single,0,0); br.addWidget(self.multi,0,1)
        br.addWidget(self.stop,1,0); br.addWidget(self.resume,1,1)
        poll_label=QLabel("Single poll timeout")
        self.poll_timeout=QDoubleSpinBox(); self.poll_timeout.setRange(0.5,30.0); self.poll_timeout.setSingleStep(0.5); self.poll_timeout.setValue(3.0); self.poll_timeout.setSuffix(" s")
        br.addWidget(poll_label,2,0); br.addWidget(self.poll_timeout,2,1)
        c.addLayout(br); sl.addWidget(ctl)

        cfg=self.card(); cf=QFormLayout(cfg); cf.addRow(self.sec("Reader Settings")); self.region=QLineEdit("FF"); self.region.setMaxLength(2); cf.addRow("Region (hex)",self.region)
        self.tx=QSpinBox(); self.tx.setRange(1,255); self.tx.setValue(1); cf.addRow("TX antenna",self.tx); self.rx=QSpinBox(); self.rx.setRange(1,255); self.rx.setValue(1); cf.addRow("RX antenna",self.rx)
        self.power=QDoubleSpinBox(); self.power.setRange(0.0,33.0); self.power.setDecimals(2); self.power.setSingleStep(0.25); self.power.setValue(30.0); self.power.setSuffix(" dBm"); self.power.setToolTip("Transmit power in dBm. Firmware sends centi-dBm."); cf.addRow("Power",self.power); self.session=QSpinBox(); self.session.setRange(0,3); cf.addRow("Session",self.session)
        sg=QGridLayout()
        for i,(t,fn) in enumerate([("Set Power",lambda:self.enqueue(f"SET_POWER,{int(round(self.power.value()*100.0))}")),("Set Antenna",lambda:self.enqueue(f"SET_ANTENNA,{self.tx.value()},{self.rx.value()}")),("Set Region",self.set_region), ("Set Session",lambda:self.enqueue(f"SET_SESSION,{self.session.value()}"))]):
            b=QPushButton(t); b.clicked.connect(fn); sg.addWidget(b,i//2,i%2)
        cf.addRow("",sg); sl.addWidget(cfg)

        info=self.card(); ig=QGridLayout(info); ig.addWidget(self.sec("Reader Information"),0,0,1,2)
        for i,(t,cmd) in enumerate([( "Version","GET_VERSION"),("Serial","GET_SERIAL"),("Temperature","GET_TEMPERATURE"),("Region","GET_REGION"),("Antenna","GET_ANTENNA"),("Power","GET_POWER"),("Protocol","GET_PROTOCOL"),("Session","GET_SESSION"),("Frequency","GET_FREQUENCY"),("Regions","GET_REGIONS")]):
            b=QPushButton(t); b.clicked.connect(lambda _,c=cmd:self.request(c)); ig.addWidget(b,1+i//2,i%2)
        sl.addWidget(info)

        flt=self.card(); fg=QGridLayout(flt); fg.addWidget(self.sec("Tag Filter"),0,0,1,2)
        self.filter_edit=QLineEdit(); self.filter_edit.setPlaceholderText("Filter EPC by text or hex…")
        self.filter_edit.textChanged.connect(self.apply_filter)
        self.filter_exact_cb=QCheckBox("Exact match")
        self.filter_exact_cb.toggled.connect(self.apply_filter)
        clear_filter=QPushButton("Clear Filter"); clear_filter.clicked.connect(self.clear_filter)
        fg.addWidget(self.filter_edit,1,0,1,2); fg.addWidget(self.filter_exact_cb,2,0); fg.addWidget(clear_filter,2,1)
        sl.addWidget(flt)

        rd=self.card(); rg=QGridLayout(rd); rg.addWidget(self.sec("Tag Read — Memory"),0,0,1,2)
        self.read_target_epc=QLineEdit(); self.read_target_epc.setPlaceholderText("Select a tag row to target it automatically")
        self.read_bank=QComboBox(); self.read_bank.addItem("TID (2)",2); self.read_bank.addItem("USER (3)",3); self.read_bank.addItem("EPC (1)",1); self.read_bank.addItem("RESERVED (0)",0); self.read_bank.setCurrentIndex(0)
        self.read_address=QSpinBox(); self.read_address.setRange(0,2147483647); self.read_address.setValue(0)
        self.read_words=QSpinBox(); self.read_words.setRange(1,96); self.read_words.setValue(2)
        self.read_tag_btn=QPushButton("Read Tag"); self.read_tag_btn.setObjectName("Primary"); self.read_tag_btn.clicked.connect(self.read_tag)
        self.read_use_selected=QPushButton("Use Selected EPC"); self.read_use_selected.clicked.connect(self.use_selected_read_epc)
        self.read_result=QLineEdit(); self.read_result.setReadOnly(True); self.read_result.setPlaceholderText("Read data will appear here")
        rg.addWidget(QLabel("Target EPC"),1,0); rg.addWidget(self.read_target_epc,1,1)
        rg.addWidget(QLabel("Memory bank"),2,0); rg.addWidget(self.read_bank,2,1)
        rg.addWidget(QLabel("Start word address"),3,0); rg.addWidget(self.read_address,3,1)
        rg.addWidget(QLabel("Word count"),4,0); rg.addWidget(self.read_words,4,1)
        rg.addWidget(self.read_use_selected,5,0); rg.addWidget(self.read_tag_btn,5,1)
        rg.addWidget(QLabel("Last data"),6,0); rg.addWidget(self.read_result,6,1)
        sl.addWidget(rd)

        wr=self.card(); wg=QGridLayout(wr); wg.addWidget(self.sec("Tag Write — Memory Data"),0,0,1,2)
        self.target_epc=QLineEdit(); self.target_epc.setPlaceholderText("Current EPC (select a tag row to fill)")
        self.write_bank=QComboBox(); self.write_bank.addItem("USER (3)",3); self.write_bank.addItem("EPC (1)",1); self.write_bank.addItem("TID (2)",2); self.write_bank.addItem("RESERVED (0)",0)
        self.write_address=QSpinBox(); self.write_address.setRange(0,2147483647); self.write_address.setValue(0)
        self.write_data=QLineEdit(); self.write_data.setPlaceholderText("Hex data, even number of digits (max 64 bytes)")
        self.write_tag_btn=QPushButton("Write Tag Data"); self.write_tag_btn.setObjectName("Primary"); self.write_tag_btn.clicked.connect(self.write_tag_data)
        self.use_selected=QPushButton("Use Selected EPC"); self.use_selected.clicked.connect(self.use_selected_epc)
        wg.addWidget(QLabel("Target EPC"),1,0); wg.addWidget(self.target_epc,1,1)
        wg.addWidget(QLabel("Memory bank"),2,0); wg.addWidget(self.write_bank,2,1)
        wg.addWidget(QLabel("Start word address"),3,0); wg.addWidget(self.write_address,3,1)
        wg.addWidget(QLabel("Write data"),4,0); wg.addWidget(self.write_data,4,1)
        wg.addWidget(self.use_selected,5,0); wg.addWidget(self.write_tag_btn,5,1)
        sl.addWidget(wr)

        opt=self.card(); ol=QVBoxLayout(opt); ol.addWidget(self.sec("Interface")); self.dedupe=QCheckBox("One row per EPC (always)"); self.dedupe.setChecked(True); self.dedupe.setEnabled(False); self.auto=QCheckBox("Auto-scroll inventory"); self.auto.setChecked(True); self.clear_start=QCheckBox("Clear inventory on START"); self.reconnect=QCheckBox("Auto reconnect"); self.reconnect.setChecked(True)
        [ol.addWidget(x) for x in (self.dedupe,self.auto,self.clear_start,self.reconnect)]; sl.addWidget(opt); sl.addStretch(); scroll.setWidget(side)

        right=QSplitter(Qt.Vertical); inv=self.card(); iv=QVBoxLayout(inv); bar=QHBoxLayout(); cl=QPushButton("Clear"); cl.clicked.connect(self.clear_inv); ex=QPushButton("Export CSV"); ex.clicked.connect(self.export_csv); ri=QPushButton("Refresh Reader Info"); ri.clicked.connect(self.request_info); self.hint=QLabel("Waiting for TAG frames…"); self.hint.setObjectName("Sub"); [bar.addWidget(x) for x in (cl,ex,ri)]; bar.addStretch(); bar.addWidget(self.hint); iv.addLayout(bar)
        self.table=QTableWidget(0,6); self.table.setHorizontalHeaderLabels(["Time","EPC","RSSI (dBm)","Antenna","Frequency (kHz)","Reads"]); self.table.setAlternatingRowColors(True); self.table.setSelectionBehavior(QTableWidget.SelectRows); self.table.verticalHeader().setVisible(False); self.table.itemSelectionChanged.connect(self.use_selected_epc); h=self.table.horizontalHeader(); h.setSectionResizeMode(0,QHeaderView.ResizeToContents); h.setSectionResizeMode(1,QHeaderView.Stretch); [h.setSectionResizeMode(c,QHeaderView.ResizeToContents) for c in (2,3,4,5)]; iv.addWidget(self.table); right.addWidget(inv)
        tabs=QTabWidget(); self.event=QPlainTextEdit(); self.event.setReadOnly(True); tabs.addTab(self.event,"Event Log"); self.raw=QPlainTextEdit(); self.raw.setReadOnly(True); tabs.addTab(self.raw,"Raw Serial")
        console=QWidget(); cv=QVBoxLayout(console); rr=QHBoxLayout(); self.command=QLineEdit(); self.command.setPlaceholderText("GET_POWER / SET_REGION,FF"); self.command.returnPressed.connect(self.manual); sb=QPushButton("Send"); sb.clicked.connect(self.manual); rr.addWidget(self.command); rr.addWidget(sb); cv.addLayout(rr); self.responses=QPlainTextEdit(); self.responses.setReadOnly(True); cv.addWidget(self.responses); tabs.addTab(console,"Command Console")
        ab=QWidget(); al=QVBoxLayout(ab); x=QLabel("Reader information requests are serialized one-at-a-time. Single Poll performs a one-shot read using the existing START/STOP host protocol; Multi Poll keeps inventory running until stopped. Tag filtering is local to the GUI and does not change what the reader captures. Tag Write writes memory data using the current EPC as the selection filter. The STM32 firmware performs up to three write attempts per click. Serial RX is line-buffered so split USB packets are reconstructed before parsing."); x.setWordWrap(True); al.addWidget(x); al.addStretch(); tabs.addTab(ab,"About"); right.addWidget(tabs); right.setStretchFactor(0,3); right.setStretchFactor(1,2)
        split.addWidget(scroll); split.addWidget(right); split.setSizes([400,1050]); outer.addWidget(split,1); self.setCentralWidget(root); self.setStatusBar(QStatusBar())

    def select_feature(self,index):
        self.stack.setCurrentIndex(index)
        for i,button in enumerate(self.nav_buttons):
            button.setChecked(i == index)

    def closeEvent(self,event):
        try:
            self.io.close()
        finally:
            event.accept()

    def set_controls(self,on):
        for w in (self.region,self.tx,self.rx,self.power,self.session,self.command,self.filter_edit,self.filter_exact_cb,self.poll_timeout,self.write_operation,self.target_epc,self.epc_edit_old,self.epc_edit_new,self.write_bank,self.write_address,self.write_data,self.use_selected,self.read_target_epc,self.read_bank,self.read_address,self.read_words,self.read_use_selected): w.setEnabled(on)
        self.write_tag_btn.setEnabled(on and not self.inventory)
        self.read_tag_btn.setEnabled(on and not self.inventory)
        self.single.setEnabled(on and not self.inventory)
        self.multi.setEnabled(on and not self.inventory)
        self.stop.setEnabled(on and self.inventory)
        self.resume.setEnabled(on and not self.inventory)

    def toggle(self):
        if self.connected:self.io.disconnect()
        else:
            p=self.port.currentData()
            if p:self.io.connect(p,int(self.baud.currentData()))
    def on_state(self,on,detail):
        self.connected=on
        if on:
            self.connect.setText("Disconnect"); self.conn.setText("Connected  "+detail); self.conn.setStyleSheet("color:#18B477;font-weight:700"); self.kv[0].setText("Online"); self.set_controls(True); self.log("Connected: "+detail)
            self.info_scheduled=True
            QTimer.singleShot(1400, self.request_info_staged)
        else:
            self.connect.setText("Connect"); self.conn.setText("Disconnected"); self.conn.setStyleSheet("color:#C34C56;font-weight:700"); self.kv[0].setText("Offline"); self.pending=None; self.epc_write_pending=''; self.queue=[]; self.pump_scheduled=False; self.info_scheduled=False; self.set_controls(False); self.log("Disconnected")
            self.single_poll_started=False
            if hasattr(self, "single_poll_timer"):
                self.single_poll_timer.stop()
            if self.reconnect.isChecked():QTimer.singleShot(1200,self.try_reconnect)
        self.statusBar().showMessage("Connected" if on else "Disconnected")
    def try_reconnect(self):
        if not self.connected and self.reconnect.isChecked() and self.port.currentData():self.io.connect(self.port.currentData(),int(self.baud.currentData()))
    def refresh_ports(self):
        old=self.port.currentData(); self.port.blockSignals(True); self.port.clear(); ps=SerialManager.ports();
        for p in ps:self.port.addItem(f"{p.device} — {p.description}",p.device)
        if not ps:self.port.addItem("No VCP ports found",None)
        else:
            target=self.settings.value("port","",type=str) or old
            for i in range(self.port.count()):
                if self.port.itemData(i)==target:self.port.setCurrentIndex(i);break
        self.port.blockSignals(False)

    def enqueue(self,cmd):
        if not self.connected:
            self.statusBar().showMessage("Connect to STM32 first.",3000)
            return
        self.queue.append(cmd)
        self.pump()

    def request(self,cmd):
        if self.inventory:
            self.statusBar().showMessage("Stop inventory before reading reader settings.",4000)
            return
        self.enqueue(cmd)

    def request_info(self):
        if self.connected and not self.inventory:
            # Avoid duplicating an existing startup/info batch.
            if any(cmd in self.queue for cmd in INFO) or self.pending:
                return
            self.queue.extend(INFO)
            self.pump()

    def request_info_staged(self):
        self.info_scheduled=False
        if self.connected and not self.inventory:
            self.queue.clear()
            self.pending=None
            self.queue.extend(INFO)
            self.pump()

    def pump(self):
        if self.pending or not self.queue or not self.connected or self.pump_scheduled:
            return

        # Give the STM32/E310 a small settling interval between host commands.
        self.pump_scheduled=True
        QTimer.singleShot(350, self._pump_now)

    def _pump_now(self):
        self.pump_scheduled=False
        if self.pending or not self.queue or not self.connected:
            return
        cmd=self.queue.pop(0)
        self.pending=[cmd,time.monotonic()]
        self.io.send(cmd)
    def on_tx(self,cmd):self.log("> "+cmd)
    def check_timeout(self):
        if self.pending and time.monotonic()-self.pending[1]>2.5:
            cmd=self.pending[0]; self.log(f"TIMEOUT waiting for {cmd}")
            if cmd.startswith('WRITE_EPC,'):
                self.epc_write_pending=''
            self.pending=None; self.pump()
    def status_poll(self):
        if self.connected and not self.inventory and not self.pending and not self.queue:self.enqueue("GET_STATUS")

    def on_raw(self,data):self.raw.appendPlainText(data.decode('utf-8','replace'))
    def on_line(self,line):
        self.responses.appendPlainText(line); kind,data=parse_line(line)
        if kind!='TAG' and self.pending:self.pending=None; self.pump()
        if kind=='TAG':self.handle_tag(data);return
        self.log("< "+line)
        if kind=='OK':
            if data=='START':self.set_inventory(True)
            elif data=='STOP':self.set_inventory(False)
            elif data=='WRITE_TAG':
                self.statusBar().showMessage('Tag memory write successful.',5000)
            elif data=='WRITE_EPC':
                self.statusBar().showMessage('Tag memory write successful.',5000)
                new_epc=self.epc_write_pending
                self.epc_write_pending=''
                self.refresh_after_epc_write(new_epc)
        elif kind=='STATUS':
            v=data.get('_csv',[]); self.set_inventory(bool(v and v[0].upper()=='INVENTORY'))
        elif kind=='TEMP':
            v=data.get('_csv',[])
            if v:self.kv[5].setText(v[0]+" °C")
        elif kind=='REGION':
            v=data.get('_csv',[])
            if v:self.region.setText(v[0])
        elif kind=='ANTENNA':self.apply(data,self.tx,'TX');self.apply(data,self.rx,'RX')
        elif kind=='POWER':
            if 'READ' in data:
                try:self.power.setValue(float(data['READ'])/100.0)
                except:pass
        elif kind=='SESSION':
            v=data.get('_csv',[])
            if v:
                try:self.session.setValue(int(v[0]))
                except:pass
        elif kind=='READ_DATA':
            bank=data.get('BANK','?'); addr=data.get('ADDR','?'); words=data.get('WORDS','?'); payload=data.get('DATA','')
            self.last_read_data=payload.upper()
            self.read_result.setText(self.last_read_data)
            self.hint.setText(f"Read bank {bank}, word {addr}, {words} word(s)")
            self.statusBar().showMessage("Tag memory read successful.",5000)
        elif kind=='ERROR':
            if data=='WRITE_EPC':
                self.epc_write_pending=''
            self.statusBar().showMessage("STM32: "+str(data),5000)
    @staticmethod
    def apply(d,w,k):
        if k in d:
            try:w.setValue(int(d[k]))
            except:pass

    def multi_poll(self):
        if self.clear_start.isChecked():
            self.clear_inv()
        self.poll_mode='MULTI'
        self.single_poll_started=False
        if hasattr(self, "single_poll_timer"):
            self.single_poll_timer.stop()
        self.queue.clear(); self.pending=None
        self.enqueue("START")

    def single_poll(self):
        if self.clear_start.isChecked():
            self.clear_inv()
        self.poll_mode='SINGLE'
        self.single_poll_started=True
        self.single_poll_timeout_ms=int(self.poll_timeout.value()*1000)
        if not hasattr(self, "single_poll_timer"):
            self.single_poll_timer=QTimer(self)
            self.single_poll_timer.setSingleShot(True)
            self.single_poll_timer.timeout.connect(self.single_poll_timeout)
        self.queue.clear(); self.pending=None
        self.enqueue("START")
        self.single_poll_timer.start(self.single_poll_timeout_ms)

    def start_inv(self):
        # Backward-compatible alias for existing UI behavior.
        self.multi_poll()

    def stop_inv(self):
        # STOP is allowed during async inventory; bypass queued reader-info requests.
        self.single_poll_started=False
        if hasattr(self, "single_poll_timer"):
            self.single_poll_timer.stop()
        self.queue.clear(); self.pending=None; self.enqueue("STOP")

    def single_poll_timeout(self):
        if self.inventory and self.poll_mode == 'SINGLE':
            self.log("Single poll timeout — stopping inventory.")
            self.stop_inv()
    def set_inventory(self,on):
        self.inventory=on
        self.kv[1].setText("INVENTORY" if on else "IDLE")
        self.single.setEnabled(self.connected and not on)
        self.multi.setEnabled(self.connected and not on)
        self.stop.setEnabled(self.connected and on)
        self.resume.setEnabled(self.connected and not on)
        self.write_tag_btn.setEnabled(self.connected and not on)
        self.read_tag_btn.setEnabled(self.connected and not on)
        if not on:
            self.single_poll_started=False
            if hasattr(self, "single_poll_timer"):
                self.single_poll_timer.stop()
    def handle_tag(self,t):
        self.total+=1
        self.epcs[t.epc]=self.epcs.get(t.epc,0)+1
        self.kv[2].setText(str(self.total))
        self.kv[3].setText(str(len(self.epcs)))

        now=time.monotonic()
        self.first_tag=self.first_tag or now
        self.kv[4].setText(f"{self.total/max(now-self.first_tag,.001):.1f}/s")
        self.hint.setText(f"Last EPC: {t.epc} • RSSI {t.rssi} dBm")

        row=self.find_row(t.epc)
        if row is None:
            row=self.table.rowCount()
            self.table.insertRow(row)

        values=[
            datetime.now().strftime('%H:%M:%S.%f')[:-3],
            t.epc,
            str(t.rssi),
            str(t.antenna),
            str(t.frequency_khz),
            str(self.epcs[t.epc])
        ]
        for c,v in enumerate(values):
            self.table.setItem(row,c,QTableWidgetItem(v))

        self.update_row_visibility(row, t.epc)

        if self.auto.isChecked():
            self.table.scrollToBottom()

        if self.poll_mode == 'SINGLE' and self.single_poll_started:
            self.single_poll_started=False
            if hasattr(self, "single_poll_timer"):
                self.single_poll_timer.stop()
            QTimer.singleShot(120, self.stop_inv)

    def update_row_visibility(self,row,epc):
        query=self.filter_edit.text().strip().upper()
        if not query:
            self.table.setRowHidden(row,False)
            return
        epc_u=epc.upper()
        match = (epc_u == query) if self.filter_exact_cb.isChecked() else (query in epc_u)
        self.table.setRowHidden(row,not match)

    def apply_filter(self,*_):
        query=self.filter_edit.text().strip().upper()
        exact=self.filter_exact_cb.isChecked()
        for r in range(self.table.rowCount()):
            item=self.table.item(r,1)
            epc=item.text().upper() if item else ''
            match = True if not query else ((epc == query) if exact else (query in epc))
            self.table.setRowHidden(r,not match)

    def clear_filter(self):
        self.filter_edit.clear()
        self.filter_exact_cb.setChecked(False)
    def use_selected_epc(self):
        rows=self.table.selectionModel().selectedRows()
        if rows:
            item=self.table.item(rows[0].row(),1)
            if item:
                epc=item.text().strip().upper()
                self.selected_epc=epc
                self.target_epc.setText(epc)
                self.epc_edit_old.setText(epc)
                self.read_target_epc.setText(epc)

    def use_selected_read_epc(self):
        rows=self.table.selectionModel().selectedRows()
        if rows:
            item=self.table.item(rows[0].row(),1)
            if item:
                self.selected_epc=item.text().strip().upper()
                self.epc_edit_old.setText(self.selected_epc)
                self.read_target_epc.setText(self.selected_epc)

    @staticmethod
    def normalize_hex(text):
        t=text.strip().replace(" ","").replace(":","").replace("0x","").replace("0X","")
        return t.upper()

    @staticmethod
    def validate_epc_value(text):
        """Return a canonical EPC or None when it is not a valid EPC value."""
        value=text
        if not value or len(value)%2 or any(c not in '0123456789abcdefABCDEF' for c in value):
            return None
        # EPC memory is addressed in 16-bit words and supports 1..31 words.
        if not 2 <= len(value)//2 <= 62:
            return None
        return value.upper()

    def update_write_mode(self, *_):
        epc_editing=self.write_operation.currentData()=='EPC_EDIT'
        for widget in (self.write_target_label,self.target_epc,self.write_bank_label,self.write_bank,self.write_address_label,self.write_address,self.write_data_label,self.write_data):
            widget.setVisible(not epc_editing)
        for widget in (self.epc_edit_old_label,self.epc_edit_old,self.epc_edit_new_label,self.epc_edit_new):
            widget.setVisible(epc_editing)
        self.write_tag_btn.setText('Write EPC' if epc_editing else 'Write Tag Data')

    def write_selected_operation(self):
        if self.write_operation.currentData()=='EPC_EDIT':
            self.write_epc()
        else:
            self.write_tag_data()

    def write_epc(self):
        if not self.connected:
            self.statusBar().showMessage("Connect to STM32 first.",3000); return
        if self.inventory:
            self.statusBar().showMessage("Stop inventory before writing a tag.",4000); return

        old_epc=self.validate_epc_value(self.selected_epc)
        if not old_epc:
            QMessageBox.warning(self,"No valid selected EPC","Select a tag row with a valid EPC first."); return

        new_epc=self.validate_epc_value(self.epc_edit_new.text())
        if not new_epc:
            QMessageBox.warning(self,"Invalid new EPC","EPC must contain hexadecimal characters only, have an even number of digits, and be 2 to 62 bytes long."); return

        answer=QMessageBox.question(
            self,
            "Confirm EPC write",
            f"Replace the selected EPC?\n\nOld EPC: {old_epc}\nNew EPC: {new_epc}",
            QMessageBox.Yes|QMessageBox.No,
            QMessageBox.No
        )
        if answer!=QMessageBox.Yes: return

        self.epc_edit_old.setText(old_epc)
        self.epc_write_pending=new_epc
        self.enqueue(f"WRITE_EPC,{old_epc},{new_epc}")

    def refresh_after_epc_write(self,new_epc):
        if not new_epc:
            return
        self.selected_epc=new_epc
        self.epc_edit_old.setText(new_epc)
        self.target_epc.setText(new_epc)
        self.read_target_epc.setText(new_epc)
        self.clear_inv()

    def read_tag(self):
        if not self.connected:
            self.statusBar().showMessage("Connect to STM32 first.",3000); return
        if self.inventory:
            self.statusBar().showMessage("Stop inventory before reading tag memory.",4000); return
        epc=self.normalize_hex(self.read_target_epc.text())
        if not epc:
            QMessageBox.warning(self,"No target EPC","Select a tag row first, or enter the EPC manually."); return
        if len(epc)%2 or len(epc)>124:
            QMessageBox.warning(self,"Invalid target EPC","Enter the selected EPC as hexadecimal."); return
        try:
            int(epc,16)
        except ValueError:
            QMessageBox.warning(self,"Invalid target EPC","EPC must contain hexadecimal digits only."); return
        bank=int(self.read_bank.currentData())
        address=self.read_address.value()
        words=self.read_words.value()
        self.selected_epc=epc
        self.epc_edit_old.setText(epc)
        self.read_target_epc.setText(epc)
        self.read_result.setText("Reading…")
        self.enqueue(f"READ_TAG,{epc},{bank},{address},{words}")

    def write_tag_data(self):
        if not self.connected:
            self.statusBar().showMessage("Connect to STM32 first.",3000); return
        if self.inventory:
            self.statusBar().showMessage("Stop inventory before writing a tag.",4000); return

        target=self.normalize_hex(self.target_epc.text())
        data=self.normalize_hex(self.write_data.text())

        if not target:
            QMessageBox.warning(self,"No target EPC","Select a tag row first, or enter the EPC manually."); return
        if len(target)%2 or len(target)>124:
            QMessageBox.warning(self,"Invalid target EPC","Enter the current EPC as hexadecimal."); return
        try:int(target,16)
        except ValueError:
            QMessageBox.warning(self,"Invalid target EPC","EPC must contain hexadecimal digits only."); return

        if not data:
            QMessageBox.warning(self,"No write data","Enter hexadecimal data to write."); return
        if len(data)%2 or len(data)>128:
            QMessageBox.warning(self,"Invalid write data","Write data must contain an even number of hex digits, up to 64 bytes."); return
        try:int(data,16)
        except ValueError:
            QMessageBox.warning(self,"Invalid write data","Write data must contain hexadecimal digits only."); return

        bank=int(self.write_bank.currentData())
        address=self.write_address.value()
        bytes_len=len(data)//2
        answer=QMessageBox.question(
            self,
            "Confirm tag write",
            f"Write {bytes_len} byte(s)?\n\nBank: {bank}\nAddress: {address}\nData: {data}\n\nThe tag matching {target} will be written.",
            QMessageBox.Yes|QMessageBox.No,
            QMessageBox.No
        )
        if answer!=QMessageBox.Yes: return

        self.selected_epc=target
        self.target_epc.setText(target)
        self.enqueue(f"WRITE_TAG,{target},{bank},{address},{data}")

    def find_row(self,epc):
        for r in range(self.table.rowCount()):
            if self.table.item(r,1) and self.table.item(r,1).text()==epc:return r
        return None
    def clear_inv(self):
        self.table.setRowCount(0)
        self.epcs={}
        self.total=0
        self.first_tag=None
        self.kv[2].setText('0')
        self.kv[3].setText('0')
        self.kv[4].setText('0.0/s')
        self.hint.setText('Waiting for TAG frames…')
    def export_csv(self):
        p,_=QFileDialog.getSaveFileName(self,'Export RFID Inventory','rfid_inventory.csv','CSV (*.csv)');
        if not p:return
        with open(p,'w',newline='',encoding='utf-8') as f:
            w=csv.writer(f);w.writerow(['Time','EPC','RSSI (dBm)','Antenna','Frequency (kHz)','Reads'])
            for r in range(self.table.rowCount()):w.writerow([self.table.item(r,c).text() if self.table.item(r,c) else '' for c in range(6)])
    def set_region(self):
        try:self.enqueue(f"SET_REGION,{int(self.region.text().replace('0x','').replace('0X',''),16):02X}")
        except:QMessageBox.warning(self,'Invalid Region','Enter a hex byte such as FF.')
    def manual(self):
        c=self.command.text().strip();
        if c:self.enqueue(c);self.command.clear()

    def log(self,msg):self.event.appendPlainText(f"[{datetime.now().strftime('%H:%M:%S.%f')[:-3]}] {msg}")
    def on_error(self,msg):self.log('ERROR: '+msg);self.statusBar().showMessage(msg,5000)
    def restore(self):
        i=self.baud.findData(self.settings.value('baud',115200,type=int));
        if i>=0:self.baud.setCurrentIndex(i)
        for k,w in [('region',self.region),('tx',self.tx),('rx',self.rx),('session',self.session)]:
            v=self.settings.value(k)
            if v is not None:
                try:w.setText(str(v)) if isinstance(w,QLineEdit) else w.setValue(int(v))
                except:pass
        saved_power=self.settings.value('power')
        if saved_power is not None:
            try:
                power_value=float(saved_power)
                if power_value > 33.0:
                    power_value /= 100.0
                self.power.setValue(power_value)
            except:pass
        self.dedupe.setChecked(True);self.read_bank.setCurrentIndex(max(0,self.read_bank.findData(self.settings.value('read_bank',2,type=int))));self.read_address.setValue(self.settings.value('read_address',0,type=int));self.read_words.setValue(self.settings.value('read_words',2,type=int));self.auto.setChecked(self.settings.value('auto',True,type=bool));self.clear_start.setChecked(self.settings.value('clear_start',False,type=bool));self.reconnect.setChecked(self.settings.value('reconnect',True,type=bool));self.filter_edit.setText(self.settings.value('filter','',type=str));self.filter_exact_cb.setChecked(self.settings.value('filter_exact',False,type=bool));self.poll_timeout.setValue(self.settings.value('poll_timeout',3.0,type=float))
    def save(self):
        self.settings.setValue('dark',self.dark);self.settings.setValue('port',self.port.currentData() or '');self.settings.setValue('baud',self.baud.currentData())
        for k,w in [('region',self.region),('tx',self.tx),('rx',self.rx),('power',self.power),('session',self.session)]:self.settings.setValue(k,w.text() if isinstance(w,QLineEdit) else w.value())
        self.settings.setValue('dedupe',True);self.settings.setValue('read_bank',self.read_bank.currentData());self.settings.setValue('read_address',self.read_address.value());self.settings.setValue('read_words',self.read_words.value());self.settings.setValue('auto',self.auto.isChecked());self.settings.setValue('clear_start',self.clear_start.isChecked());self.settings.setValue('reconnect',self.reconnect.isChecked());self.settings.setValue('filter',self.filter_edit.text());self.settings.setValue('filter_exact',self.filter_exact_cb.isChecked());self.settings.setValue('poll_timeout',self.poll_timeout.value())
    def apply_theme(self):QApplication.instance().setStyleSheet(theme.DARK if self.dark else theme.LIGHT);self.theme_btn.setText('Bright' if self.dark else 'Dark');self.save()
    def set_theme(self,d):self.dark=d;self.apply_theme();self.log('Theme changed to '+('Dark' if d else 'Bright'))
    def closeEvent(self,e):self.save();self.io.close();e.accept()

app=QApplication(sys.argv);app.setStyle('Fusion');w=Main();w.show();sys.exit(app.exec())
