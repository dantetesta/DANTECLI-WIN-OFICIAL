import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQml
import Backend 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 1320
    height: 820
    title: "DANTE CLI"
    color: "#0E0E11"

    readonly property color cBg:      "#0E0E11"
    readonly property color cPanel:   "#161619"
    readonly property color cPanel2:  "#1E1E24"
    readonly property color cTermBg:  "#0A0A0C"
    readonly property color cAccent:  "#2D6BF0"
    readonly property color cText:    "#E8E8EC"
    readonly property color cDim:     "#7E7E88"
    readonly property color cBorder:  "#262630"
    readonly property color cField:   "#0F0F14"

    property int sideMode: 1
    property int selFile: -1
    property int gridCols: 1
    property int gridRows: 1
    property int curTerm: 0
    property bool splitMode: false
    property var focusedTerm: null
    property int dragTab: -1    // aba sendo arrastada (reorder na barra), -1 = nenhuma
    property int dragSlot: -1   // pane do split sendo arrastado (troca slots), -1 = nenhum
    property int nextTid: 0     // gerador de id estável de terminal

    property var dlgModel: null
    property int dlgIndex: -1
    property int credIndex: -1
    // menu de contexto da árvore de pastas
    property int ctxRow: -1; property string ctxPath: ""; property string ctxName: ""; property bool ctxDir: false
    property string nameMode: ""; property string nameTarget: ""   // diálogo "renomear / novo arquivo / nova pasta"
    readonly property var credTypes: ["SSH", "FTP", "SFTP", "MySQL", "API Key", "Custom"]

    // ===== dados =====
    ListModel { id: favModel
        ListElement { title: "Home"; detail: "~" }
        ListElement { title: "Projetos"; detail: "~/Projetos" } }
    ListModel { id: snipModel
        ListElement { title: "git build"; detail: "git pull && npm run build" }
        ListElement { title: "docker up"; detail: "docker compose up -d" } }
    ListModel { id: keyModel
        dynamicRoles: true
        Component.onCompleted: {
            append({ name: "Cliente A — VPS", type: "SSH", notes: "",
                fields: [ {key:"host",value:"189.45.10.2",secret:false},{key:"port",value:"22",secret:false},{key:"user",value:"root",secret:false},{key:"password",value:"s3nh4",secret:true},{key:"key_path",value:"~/.ssh/id_rsa",secret:false} ] })
            append({ name: "OpenAI", type: "API Key", notes: "",
                fields: [ {key:"api_key",value:"sk-proj-xxxx",secret:true} ] })
        } }
    ListModel { id: dlgFields }
    ListModel { id: sessionsModel; Component.onCompleted: append({ tid: win.nextTid++ }) }
    Instantiator { id: terms; model: sessionsModel; delegate: TerminalController { Component.onCompleted: start() } }
    // Split = grade fixa de slots; cada slot referencia um terminal por tid (-1 = vazio).
    ListModel { id: slotModel }
    ListModel { id: filesModel }   // árvore lazy: cada item {fname, dir, depth, open, fpath}

    function reloadTree() {
        filesModel.clear()
        var es = Files.list(Files.root)
        for (var i = 0; i < es.length; i++)
            filesModel.append({ fname: es[i].name, dir: es[i].dir, depth: 0, open: false, fpath: es[i].path })
        win.selFile = -1
    }
    function toggleRow(idx) {
        var row = filesModel.get(idx)
        win.selFile = idx
        if (!row.dir) return
        if (row.open) {
            var n = 0
            for (var j = idx + 1; j < filesModel.count; j++) {
                if (filesModel.get(j).depth > row.depth) n++; else break
            }
            if (n > 0) filesModel.remove(idx + 1, n)
            filesModel.setProperty(idx, "open", false)
        } else {
            var es = Files.list(row.fpath)
            for (var k = 0; k < es.length; k++)
                filesModel.insert(idx + 1 + k, { fname: es[k].name, dir: es[k].dir, depth: row.depth + 1, open: false, fpath: es[k].path })
            filesModel.setProperty(idx, "open", true)
        }
    }
    Component.onCompleted: reloadTree()
    Connections { target: Files
        function onRootChanged() { win.reloadTree() }
        function onShowHiddenChanged() { win.reloadTree() }
    }

    // ===== funções =====
    function openItem(lm, idx, modeName) {
        win.dlgModel = lm; win.dlgIndex = idx
        dlgTitleLbl.text = (idx < 0 ? "Adicionar em " : "Editar — ") + modeName
        fTitle.text = idx >= 0 ? lm.get(idx).title : ""
        fDetail.text = idx >= 0 ? lm.get(idx).detail : ""
        itemDialog.open()
    }
    function defaultFields(type) {
        if (type === "SSH") return [ {key:"host",value:"",secret:false},{key:"port",value:"22",secret:false},{key:"user",value:"",secret:false},{key:"password",value:"",secret:true},{key:"key_path",value:"",secret:false} ]
        if (type === "FTP") return [ {key:"host",value:"",secret:false},{key:"port",value:"21",secret:false},{key:"user",value:"",secret:false},{key:"password",value:"",secret:true} ]
        if (type === "SFTP") return [ {key:"host",value:"",secret:false},{key:"port",value:"22",secret:false},{key:"user",value:"",secret:false},{key:"password",value:"",secret:true} ]
        if (type === "MySQL") return [ {key:"host",value:"",secret:false},{key:"port",value:"3306",secret:false},{key:"user",value:"",secret:false},{key:"password",value:"",secret:true},{key:"database",value:"",secret:false} ]
        if (type === "API Key") return [ {key:"api_key",value:"",secret:true},{key:"base_url",value:"",secret:false} ]
        return [ {key:"campo",value:"",secret:false} ]
    }
    function loadFields(arr) { dlgFields.clear(); for (var i = 0; i < arr.length; ++i) dlgFields.append({ key: arr[i].key, value: arr[i].value, secret: arr[i].secret === true, revealed: false }) }
    function applyType(type) { credTypeSel.value = type; loadFields(defaultFields(type)) }
    function openCred(idx) {
        win.credIndex = idx
        if (idx < 0) { credNameField.text = ""; credNotesField.text = ""; credTypeSel.value = "SSH"; loadFields(defaultFields("SSH")) }
        else { var c = keyModel.get(idx); credNameField.text = c.name; credNotesField.text = c.notes; credTypeSel.value = c.type; loadFields(c.fields) }
        credDialog.open()
    }
    function saveCred() {
        if (credNameField.text.trim().length === 0) return
        var arr = []
        for (var i = 0; i < dlgFields.count; ++i) { var f = dlgFields.get(i); arr.push({ key: f.key, value: f.value, secret: f.secret === true }) }
        var obj = { name: credNameField.text, type: credTypeSel.value, notes: credNotesField.text, fields: arr }
        if (win.credIndex < 0) keyModel.append(obj); else keyModel.set(win.credIndex, obj)
        credDialog.close()
    }
    function credIcon(type) {
        if (type === "SSH") return "terminal"
        if (type === "FTP" || type === "SFTP") return "server"
        if (type === "MySQL") return "database"
        if (type === "API Key") return "key"
        return "lock"
    }
    // reorder de terminais: arrasta um pane/aba sobre outro -> move no model (ghost + DropArea)
    // ---- terminais por tid estável (abas) + slots do split ----
    function makeTerm() { var t = win.nextTid++; sessionsModel.append({ tid: t }); return t }
    function tabIndexForTid(tid) {
        for (var i = 0; i < sessionsModel.count; i++) if (sessionsModel.get(i).tid === tid) return i
        return -1
    }
    function ctrlForTid(tid) { var i = tabIndexForTid(tid); return i >= 0 ? terms.objectAt(i) : null }
    function addTerm() { makeTerm(); win.curTerm = sessionsModel.count - 1 }
    function openTerminalAt(path) {
        var t = makeTerm(); win.splitMode = false; win.curTerm = win.tabIndexForTid(t)
        if (path && path.length) Qt.callLater(function () { var c = win.ctrlForTid(t); if (c) c.send("cd \"" + path + "\"\r") })
    }
    // fecha a ABA (mata o terminal): some das abas e esvazia qualquer slot que o usava.
    function closeTab(i) {
        if (sessionsModel.count <= 1) return
        var tid = sessionsModel.get(i).tid
        sessionsModel.remove(i)
        for (var s = 0; s < slotModel.count; s++) if (slotModel.get(s).tid === tid) slotModel.setProperty(s, "tid", -1)
        if (win.curTerm >= sessionsModel.count) win.curTerm = sessionsModel.count - 1
    }
    // reordena a barra de abas (drag horizontal)
    function reorderTab(to) {
        if (win.dragTab < 0 || win.dragTab === to || to < 0 || to >= sessionsModel.count) return
        sessionsModel.move(win.dragTab, to, 1)
        if (win.curTerm === win.dragTab) win.curTerm = to
        win.dragTab = to
    }
    function applyGrid(c, r) {
        win.gridCols = c; win.gridRows = r
        var target = c * r
        while (sessionsModel.count < target) makeTerm()   // garante terminais p/ preencher
        slotModel.clear()
        for (var p = 0; p < target; p++)
            slotModel.append({ tid: p < sessionsModel.count ? sessionsModel.get(p).tid : -1 })
        win.splitMode = true
    }
    function closeSlot(s) { slotModel.setProperty(s, "tid", -1) }            // só esvazia o slot
    function fillSlot(s) { slotModel.setProperty(s, "tid", makeTerm()) }     // novo terminal no slot vazio
    function swapSlots(a, b) {
        if (a < 0 || b < 0 || a === b) return
        var ta = slotModel.get(a).tid, tb = slotModel.get(b).tid
        slotModel.setProperty(a, "tid", tb); slotModel.setProperty(b, "tid", ta)
    }
    // ---- menu de contexto da árvore ----
    function ctxAction(act) {
        var par = win.ctxDir ? win.ctxPath : Files.parentDir(win.ctxPath)
        if (act === "expand") win.toggleRow(win.ctxRow)
        else if (act === "terminal") win.openTerminalAt(win.ctxPath)
        else if (act === "root") Files.root = par
        else if (act === "fav") favModel.append({ title: win.ctxName, detail: win.ctxPath })
        else if (act === "copy") Files.copyPath(win.ctxPath)
        else if (act === "reveal") Files.revealInOS(win.ctxPath)
        else if (act === "trash") { if (Files.trashPath(win.ctxPath)) win.reloadTree() }
        else if (act === "rename") { win.nameMode = "rename"; win.nameTarget = win.ctxPath; nameField.text = win.ctxName; nameDlg.open() }
        else if (act === "newfile") { win.nameMode = "newfile"; win.nameTarget = par; nameField.text = ""; nameDlg.open() }
        else if (act === "newfolder") { win.nameMode = "newfolder"; win.nameTarget = par; nameField.text = ""; nameDlg.open() }
        ctxMenu.close()
    }
    function nameConfirm() {
        var ok = false
        if (win.nameMode === "rename") ok = Files.renamePath(win.nameTarget, nameField.text)
        else if (win.nameMode === "newfile") ok = Files.makeFile(win.nameTarget, nameField.text)
        else if (win.nameMode === "newfolder") ok = Files.makeFolder(win.nameTarget, nameField.text)
        if (ok) win.reloadTree()
        nameDlg.close()
    }

    // ===== componentes =====
    component Pill: Rectangle {
        property alias label: t.text; property color fg: "#B8B8C0"
        radius: 6; color: "#23232A"; implicitWidth: t.implicitWidth + 16; implicitHeight: 22
        Text { id: t; anchors.centerIn: parent; color: parent.fg; font.pixelSize: 11; font.bold: true }
    }
    component TBtn: Rectangle {
        id: tb; property string glyph: ""; property string iconKind: ""; property string label: ""; property color tint: win.cText
        signal clicked()
        implicitWidth: r.implicitWidth + 18; implicitHeight: 30; radius: 7
        color: ma.containsMouse ? "#2A2A34" : "transparent"
        Behavior on color { ColorAnimation { duration: 110 } }
        scale: ma.pressed ? 0.94 : 1.0
        Behavior on scale { NumberAnimation { duration: 90 } }
        Row { id: r; anchors.centerIn: parent; spacing: 6
            Icon { visible: tb.iconKind.length > 0; width: 16; height: 16; kind: tb.iconKind; stroke: tb.tint; anchors.verticalCenter: parent.verticalCenter }
            Text { visible: tb.iconKind.length === 0 && tb.glyph.length > 0; text: tb.glyph; color: tb.tint; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
            Text { visible: tb.label.length > 0; text: tb.label; color: tb.tint; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter } }
        MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: tb.clicked() }
    }
    // Ícones vetoriais (sem emoji). kind: dots|eye|eyeoff|reload|folder|file|chevron
    component Icon: Canvas {
        property string kind: ""; property color stroke: cDim; property real sw: 1.5; property bool down: false
        implicitWidth: 16; implicitHeight: 16
        onKindChanged: requestPaint(); onStrokeChanged: requestPaint(); onDownChanged: requestPaint()
        onPaint: {
            var c = getContext("2d"); c.reset()
            c.strokeStyle = stroke; c.fillStyle = stroke; c.lineWidth = sw; c.lineCap = "round"; c.lineJoin = "round"
            var w = width, h = height
            if (kind === "dots") {
                for (var i = 0; i < 3; i++) { c.beginPath(); c.arc(w * 0.22 + i * (w * 0.28), h * 0.5, 1.5, 0, 2 * Math.PI); c.fill() }
            } else if (kind === "eye" || kind === "eyeoff") {
                c.beginPath(); c.moveTo(w * 0.08, h * 0.5)
                c.bezierCurveTo(w * 0.3, h * 0.16, w * 0.7, h * 0.16, w * 0.92, h * 0.5)
                c.bezierCurveTo(w * 0.7, h * 0.84, w * 0.3, h * 0.84, w * 0.08, h * 0.5); c.stroke()
                c.beginPath(); c.arc(w * 0.5, h * 0.5, w * 0.13, 0, 2 * Math.PI); c.stroke()
                if (kind === "eyeoff") { c.beginPath(); c.moveTo(w * 0.16, h * 0.16); c.lineTo(w * 0.84, h * 0.84); c.stroke() }
            } else if (kind === "reload") {
                c.beginPath(); c.arc(w * 0.5, h * 0.5, w * 0.3, Math.PI * 0.55, Math.PI * 1.9); c.stroke()
                var ax = w * 0.5 + w * 0.3 * Math.cos(Math.PI * 1.9), ay = h * 0.5 + w * 0.3 * Math.sin(Math.PI * 1.9)
                c.beginPath(); c.moveTo(ax, ay); c.lineTo(ax - w * 0.16, ay); c.moveTo(ax, ay); c.lineTo(ax, ay + h * 0.16); c.stroke()
            } else if (kind === "folder") {
                c.beginPath(); c.moveTo(w * 0.1, h * 0.34); c.lineTo(w * 0.4, h * 0.34); c.lineTo(w * 0.48, h * 0.44)
                c.lineTo(w * 0.9, h * 0.44); c.lineTo(w * 0.9, h * 0.76); c.lineTo(w * 0.1, h * 0.76); c.closePath()
                c.globalAlpha = 0.16; c.fill(); c.globalAlpha = 1; c.stroke()
            } else if (kind === "file") {
                c.beginPath(); c.moveTo(w * 0.26, h * 0.16); c.lineTo(w * 0.58, h * 0.16); c.lineTo(w * 0.74, h * 0.32)
                c.lineTo(w * 0.74, h * 0.84); c.lineTo(w * 0.26, h * 0.84); c.closePath(); c.stroke()
                c.beginPath(); c.moveTo(w * 0.58, h * 0.16); c.lineTo(w * 0.58, h * 0.32); c.lineTo(w * 0.74, h * 0.32); c.stroke()
            } else if (kind === "chevron") {
                if (down) { c.beginPath(); c.moveTo(w * 0.3, h * 0.42); c.lineTo(w * 0.5, h * 0.62); c.lineTo(w * 0.7, h * 0.42); c.stroke() }
                else { c.beginPath(); c.moveTo(w * 0.42, h * 0.3); c.lineTo(w * 0.62, h * 0.5); c.lineTo(w * 0.42, h * 0.7); c.stroke() }
            } else if (kind === "star") {
                var cx = w * 0.5, cy = h * 0.52, R = w * 0.4, r0 = w * 0.17
                c.beginPath()
                for (var si = 0; si < 10; si++) {
                    var ang = -Math.PI / 2 + si * Math.PI / 5, rad = (si % 2 === 0) ? R : r0
                    var sx = cx + rad * Math.cos(ang), sy = cy + rad * Math.sin(ang)
                    if (si === 0) c.moveTo(sx, sy); else c.lineTo(sx, sy)
                }
                c.closePath(); c.globalAlpha = 0.16; c.fill(); c.globalAlpha = 1; c.stroke()
            } else if (kind === "code") {
                c.beginPath(); c.moveTo(w * 0.38, h * 0.28); c.lineTo(w * 0.2, h * 0.5); c.lineTo(w * 0.38, h * 0.72); c.stroke()
                c.beginPath(); c.moveTo(w * 0.62, h * 0.28); c.lineTo(w * 0.8, h * 0.5); c.lineTo(w * 0.62, h * 0.72); c.stroke()
            } else if (kind === "key") {
                c.beginPath(); c.arc(w * 0.34, h * 0.5, w * 0.16, 0, 2 * Math.PI); c.stroke()
                c.beginPath(); c.moveTo(w * 0.49, h * 0.5); c.lineTo(w * 0.86, h * 0.5)
                c.moveTo(w * 0.72, h * 0.5); c.lineTo(w * 0.72, h * 0.66); c.moveTo(w * 0.84, h * 0.5); c.lineTo(w * 0.84, h * 0.63); c.stroke()
            } else if (kind === "terminal") {
                c.beginPath(); c.moveTo(w * 0.22, h * 0.32); c.lineTo(w * 0.4, h * 0.5); c.lineTo(w * 0.22, h * 0.68); c.stroke()
                c.beginPath(); c.moveTo(w * 0.46, h * 0.68); c.lineTo(w * 0.76, h * 0.68); c.stroke()
            } else if (kind === "trash") {
                c.beginPath(); c.moveTo(w * 0.24, h * 0.3); c.lineTo(w * 0.76, h * 0.3); c.stroke()
                c.beginPath(); c.moveTo(w * 0.4, h * 0.3); c.lineTo(w * 0.42, h * 0.22); c.lineTo(w * 0.58, h * 0.22); c.lineTo(w * 0.6, h * 0.3); c.stroke()
                c.beginPath(); c.moveTo(w * 0.3, h * 0.3); c.lineTo(w * 0.34, h * 0.8); c.lineTo(w * 0.66, h * 0.8); c.lineTo(w * 0.7, h * 0.3); c.stroke()
                c.beginPath(); c.moveTo(w * 0.43, h * 0.4); c.lineTo(w * 0.45, h * 0.7); c.moveTo(w * 0.57, h * 0.4); c.lineTo(w * 0.55, h * 0.7); c.stroke()
            } else if (kind === "plus") {
                c.beginPath(); c.moveTo(w * 0.5, h * 0.26); c.lineTo(w * 0.5, h * 0.74); c.moveTo(w * 0.26, h * 0.5); c.lineTo(w * 0.74, h * 0.5); c.stroke()
            } else if (kind === "mic") {
                c.beginPath(); c.moveTo(w * 0.38, h * 0.42); c.lineTo(w * 0.38, h * 0.28)
                c.arc(w * 0.5, h * 0.28, w * 0.12, Math.PI, 2 * Math.PI); c.lineTo(w * 0.62, h * 0.42)
                c.arc(w * 0.5, h * 0.42, w * 0.12, 0, Math.PI); c.stroke()
                c.beginPath(); c.arc(w * 0.5, h * 0.42, w * 0.22, 0.12 * Math.PI, 0.88 * Math.PI); c.stroke()
                c.beginPath(); c.moveTo(w * 0.5, h * 0.64); c.lineTo(w * 0.5, h * 0.8); c.moveTo(w * 0.38, h * 0.8); c.lineTo(w * 0.62, h * 0.8); c.stroke()
            } else if (kind === "spark") {
                c.beginPath(); c.moveTo(w * 0.5, h * 0.16); c.lineTo(w * 0.58, h * 0.42); c.lineTo(w * 0.84, h * 0.5); c.lineTo(w * 0.58, h * 0.58); c.lineTo(w * 0.5, h * 0.84); c.lineTo(w * 0.42, h * 0.58); c.lineTo(w * 0.16, h * 0.5); c.lineTo(w * 0.42, h * 0.42); c.closePath()
                c.globalAlpha = 0.2; c.fill(); c.globalAlpha = 1; c.stroke()
            } else if (kind === "x") {
                c.beginPath(); c.moveTo(w * 0.3, h * 0.3); c.lineTo(w * 0.7, h * 0.7); c.moveTo(w * 0.7, h * 0.3); c.lineTo(w * 0.3, h * 0.7); c.stroke()
            } else if (kind === "grip") {
                for (var gx = 0; gx < 2; gx++) for (var gy = 0; gy < 3; gy++) {
                    c.beginPath(); c.arc(w * (0.4 + gx * 0.2), h * (0.3 + gy * 0.2), 1.1, 0, 2 * Math.PI); c.fill()
                }
            } else if (kind === "database") {
                c.beginPath(); c.moveTo(w * 0.25, h * 0.3); c.bezierCurveTo(w * 0.25, h * 0.2, w * 0.75, h * 0.2, w * 0.75, h * 0.3)
                c.bezierCurveTo(w * 0.75, h * 0.4, w * 0.25, h * 0.4, w * 0.25, h * 0.3); c.stroke()
                c.beginPath(); c.moveTo(w * 0.25, h * 0.3); c.lineTo(w * 0.25, h * 0.7)
                c.bezierCurveTo(w * 0.25, h * 0.8, w * 0.75, h * 0.8, w * 0.75, h * 0.7); c.lineTo(w * 0.75, h * 0.3); c.stroke()
                c.beginPath(); c.moveTo(w * 0.25, h * 0.5); c.bezierCurveTo(w * 0.25, h * 0.6, w * 0.75, h * 0.6, w * 0.75, h * 0.5); c.stroke()
            } else if (kind === "server") {
                c.strokeRect(w * 0.22, h * 0.26, w * 0.56, h * 0.2)
                c.strokeRect(w * 0.22, h * 0.54, w * 0.56, h * 0.2)
                c.beginPath(); c.arc(w * 0.66, h * 0.36, 1.2, 0, 2 * Math.PI); c.fill()
                c.beginPath(); c.arc(w * 0.66, h * 0.64, 1.2, 0, 2 * Math.PI); c.fill()
            } else if (kind === "globe") {
                c.beginPath(); c.arc(w * 0.5, h * 0.5, w * 0.32, 0, 2 * Math.PI); c.stroke()
                c.beginPath(); c.moveTo(w * 0.5, h * 0.18); c.bezierCurveTo(w * 0.3, h * 0.35, w * 0.3, h * 0.65, w * 0.5, h * 0.82)
                c.bezierCurveTo(w * 0.7, h * 0.65, w * 0.7, h * 0.35, w * 0.5, h * 0.18); c.stroke()
                c.beginPath(); c.moveTo(w * 0.18, h * 0.5); c.lineTo(w * 0.82, h * 0.5); c.stroke()
            } else if (kind === "lock") {
                c.strokeRect(w * 0.3, h * 0.46, w * 0.4, h * 0.34)
                c.beginPath(); c.arc(w * 0.5, h * 0.46, w * 0.13, Math.PI, 2 * Math.PI); c.stroke()
            } else if (kind === "pencil") {
                c.beginPath(); c.moveTo(w * 0.68, h * 0.22); c.lineTo(w * 0.8, h * 0.34); c.lineTo(w * 0.36, h * 0.78); c.lineTo(w * 0.22, h * 0.8); c.lineTo(w * 0.24, h * 0.66); c.closePath(); c.stroke()
                c.beginPath(); c.moveTo(w * 0.6, h * 0.3); c.lineTo(w * 0.72, h * 0.42); c.stroke()
            } else if (kind === "split") {
                c.strokeRect(w * 0.2, h * 0.2, w * 0.6, h * 0.6)
                c.beginPath(); c.moveTo(w * 0.5, h * 0.2); c.lineTo(w * 0.5, h * 0.8); c.moveTo(w * 0.2, h * 0.5); c.lineTo(w * 0.8, h * 0.5); c.stroke()
            } else if (kind === "exit") {
                c.beginPath(); c.moveTo(w * 0.46, h * 0.24); c.lineTo(w * 0.26, h * 0.24); c.lineTo(w * 0.26, h * 0.76); c.lineTo(w * 0.46, h * 0.76); c.stroke()
                c.beginPath(); c.moveTo(w * 0.56, h * 0.34); c.lineTo(w * 0.74, h * 0.5); c.lineTo(w * 0.56, h * 0.66); c.moveTo(w * 0.74, h * 0.5); c.lineTo(w * 0.44, h * 0.5); c.stroke()
            } else if (kind === "gear") {
                var gcx = w * 0.5, gcy = h * 0.5
                for (var gi = 0; gi < 8; gi++) {
                    var ga = gi * Math.PI / 4
                    c.beginPath(); c.moveTo(gcx + Math.cos(ga) * w * 0.26, gcy + Math.sin(ga) * w * 0.26)
                    c.lineTo(gcx + Math.cos(ga) * w * 0.38, gcy + Math.sin(ga) * w * 0.38); c.stroke()
                }
                c.beginPath(); c.arc(gcx, gcy, w * 0.22, 0, 2 * Math.PI); c.stroke()
                c.beginPath(); c.arc(gcx, gcy, w * 0.09, 0, 2 * Math.PI); c.stroke()
            }
        }
    }
    component IconBtn: Rectangle {
        property string kind: ""; property color tint: cDim; property color tintHover: cText
        signal clicked()
        implicitWidth: 26; implicitHeight: 26; radius: 6
        color: ibm.containsMouse ? "#23232E" : "transparent"
        Behavior on color { ColorAnimation { duration: 100 } }
        Icon { anchors.centerIn: parent; kind: parent.kind; stroke: ibm.containsMouse ? parent.tintHover : parent.tint }
        MouseArea { id: ibm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: parent.clicked() }
    }
    component FField: Rectangle {
        property alias text: tf.text; property alias placeholder: tf.placeholderText; property bool secret: false
        radius: 8; color: cField; border.color: tf.activeFocus ? cAccent : "#33333E"; border.width: 1; implicitHeight: 38
        TextField { id: tf; anchors.fill: parent; leftPadding: 10; rightPadding: 10; color: cText; font.pixelSize: 14
            placeholderTextColor: "#5A5A64"; echoMode: parent.secret ? TextInput.Password : TextInput.Normal; background: Item {} }
    }
    component Stepper: Rectangle {
        id: step
        property int value: 1; property int minv: 1; property int maxv: 4
        signal changed(int v)
        implicitWidth: 130; implicitHeight: 38; radius: 8; color: cField; border.color: "#33333E"; border.width: 1
        RowLayout { anchors.fill: parent; anchors.margins: 3; spacing: 0
            Rectangle { implicitWidth: 36; Layout.fillHeight: true; radius: 6; color: smm.containsMouse ? "#2A2A36" : "transparent"
                Text { anchors.centerIn: parent; text: "–"; color: cText; font.pixelSize: 17 }
                MouseArea { id: smm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: step.changed(Math.max(step.minv, step.value - 1)) } }
            Text { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; text: step.value; color: cText; font.pixelSize: 15; font.bold: true }
            Rectangle { implicitWidth: 36; Layout.fillHeight: true; radius: 6; color: spm.containsMouse ? "#2A2A36" : "transparent"
                Text { anchors.centerIn: parent; text: "+"; color: cText; font.pixelSize: 16 }
                MouseArea { id: spm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: step.changed(Math.min(step.maxv, step.value + 1)) } }
        }
    }
    component CrudList: ColumnLayout {
        id: cl; property var lm; property string title: ""; property string iconKind: "star"; property color glyphColor: "#F0C24A"
        signal rowActivated(int i)
        spacing: 0
        RowLayout { Layout.fillWidth: true; Layout.leftMargin: 14; Layout.rightMargin: 12; Layout.topMargin: 8; Layout.bottomMargin: 4
            Text { text: cl.title; color: cText; font.pixelSize: 13; font.bold: true }
            Item { Layout.fillWidth: true }
            Rectangle { implicitWidth: addr.implicitWidth + 16; implicitHeight: 26; radius: 7; color: addm.containsMouse ? "#3576F5" : "#2D6BF0"
                Row { id: addr; anchors.centerIn: parent; spacing: 4
                    Text { text: "+"; color: "white"; font.pixelSize: 14; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "Novo"; color: "white"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter } }
                MouseArea { id: addm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.openItem(cl.lm, -1, cl.title) } } }
        ListView { Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: cl.lm
            delegate: Rectangle {
                required property int index; required property string title; required property string detail
                width: ListView.view.width; height: 42; color: rma.containsMouse ? "#20202A" : "transparent"
                MouseArea { id: rma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: cl.rowActivated(index) }
                RowLayout { anchors.fill: parent; anchors.leftMargin: 14; anchors.rightMargin: 8; spacing: 9
                    Icon { Layout.alignment: Qt.AlignVCenter; width: 17; height: 17; kind: cl.iconKind; stroke: cl.glyphColor }
                    ColumnLayout { spacing: 1; Layout.fillWidth: true
                        Text { text: title; color: cText; font.pixelSize: 13; elide: Text.ElideRight; Layout.fillWidth: true }
                        Text { text: detail; color: cDim; font.pixelSize: 10; elide: Text.ElideRight; Layout.fillWidth: true; visible: detail.length > 0 } }
                    Rectangle { implicitWidth: 24; implicitHeight: 24; radius: 6; color: edm.containsMouse ? "#2A2A36" : "transparent"
                        Icon { anchors.centerIn: parent; width: 14; height: 14; kind: "pencil"; stroke: edm.containsMouse ? cAccent : cDim }
                        MouseArea { id: edm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.openItem(cl.lm, index, cl.title) } }
                    Rectangle { implicitWidth: 24; implicitHeight: 24; radius: 6; color: dlm.containsMouse ? "#43232A" : "transparent"
                        Icon { anchors.centerIn: parent; width: 13; height: 13; kind: "x"; stroke: dlm.containsMouse ? "#FF6A78" : cDim }
                        MouseArea { id: dlm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: cl.lm.remove(index) } }
                }
            } }
    }
    component TermView: Rectangle {
        property var ctrl
        property string title: "Terminal"
        property int slotIndex: -1
        signal closeReq()
        radius: 10; color: cTermBg; clip: true
        border.width: 1.5
        border.color: (win.dragSlot >= 0 && win.dragSlot === slotIndex) ? "#15C2D6"
                      : ((ctrl && win.focusedTerm === ctrl) ? cAccent : "#2A3550")
        opacity: (win.dragSlot >= 0 && win.dragSlot === slotIndex) ? 0.55 : 1.0
        ColumnLayout { anchors.fill: parent; spacing: 0
            Rectangle { id: termHead; Layout.fillWidth: true; height: 34; radius: 9
                gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0.0; color: "#1E5FD0" } GradientStop { position: 1.0; color: "#2E7BF0" } }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 9; color: "#256BE0" }
                // arrastar a barra do título reordena os terminais (só no split). Fica ATRÁS dos botões.
                MouseArea {
                    id: dragMa; anchors.fill: parent; enabled: win.splitMode && slotIndex >= 0
                    cursorShape: enabled ? Qt.OpenHandCursor : Qt.ArrowCursor
                    property real px: 0; property real py: 0; property bool moved: false
                    onPressed: (m) => { px = m.x; py = m.y; moved = false }
                    onPositionChanged: (m) => {
                        if (!enabled) return
                        if (!moved && (Math.abs(m.x - px) > 6 || Math.abs(m.y - py) > 6)) {
                            moved = true; win.dragSlot = slotIndex; dragGhost.label = title; dragGhost.Drag.active = true
                        }
                        if (moved) { var p = mapToItem(dragLayer, m.x, m.y); dragGhost.x = p.x - dragGhost.width / 2; dragGhost.y = p.y - dragGhost.height / 2 }
                    }
                    onReleased: { if (moved) { dragGhost.Drag.drop(); dragGhost.Drag.active = false; win.dragSlot = -1; moved = false } }
                }
                RowLayout { anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 6; spacing: 6
                    Icon { visible: win.splitMode; width: 14; height: 14; kind: "grip"; stroke: "#BcD4FF" }
                    Icon { width: 14; height: 14; kind: "terminal"; stroke: "white" }
                    Text { text: title; color: "white"; font.pixelSize: 12; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Rectangle { radius: 5; implicitWidth: avp.implicitWidth + 12; implicitHeight: 18; color: "#15C2D6"
                        Text { id: avp; anchors.centerIn: parent; text: (ctrl && ctrl.running) ? "ATIVO" : "—"; color: "#062B30"; font.pixelSize: 10; font.bold: true } }
                    Rectangle { implicitWidth: 22; implicitHeight: 22; radius: 5; color: clm.containsMouse ? "#1B4FB0" : "transparent"
                        Icon { anchors.centerIn: parent; width: 14; height: 14; kind: "trash"; stroke: "white" }
                        MouseArea { id: clm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: if (ctrl) ctrl.clearOutput() } }
                    Rectangle { visible: win.splitMode; implicitWidth: 22; implicitHeight: 22; radius: 5; color: xcm.containsMouse ? "#C23A4A" : "transparent"
                        Icon { anchors.centerIn: parent; width: 13; height: 13; kind: "x"; stroke: "white" }
                        MouseArea { id: xcm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: closeReq() } }
                }
            }
            ScrollView { Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                TextArea { readOnly: true; wrapMode: TextArea.WrapAnywhere; text: ctrl ? ctrl.output : ""; color: "#D6D6DA"
                    font.family: "Consolas"; font.pixelSize: 12; leftPadding: 12; topPadding: 8
                    onTextChanged: cursorPosition = length
                    background: Rectangle { color: cTermBg } } }
            Rectangle { Layout.fillWidth: true; height: 36; color: "#0C0C0F"
                RowLayout { anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 10; spacing: 6
                    Text { text: "›"; color: "#5BD675"; font.pixelSize: 15; font.bold: true }
                    TextField { id: cmdField; Layout.fillWidth: true; placeholderText: "comando + Enter"; color: cText; placeholderTextColor: "#5A5A64"
                        font.family: "Consolas"; font.pixelSize: 12
                        onActiveFocusChanged: if (activeFocus && ctrl) win.focusedTerm = ctrl
                        onAccepted: { if (ctrl) ctrl.send(text + "\r"); text = "" }
                        background: Rectangle { color: "transparent" }
                        // texto transcrito do mic entra aqui (não envia: usuário revisa e dá Enter)
                        Connections { target: ctrl
                            function onInsertRequested(t) { cmdField.text = (cmdField.text.length ? cmdField.text + " " : "") + t; cmdField.forceActiveFocus() } } } }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ============ SIDEBAR ============
        Rectangle {
            Layout.preferredWidth: 262; Layout.fillHeight: true; color: cPanel
            ColumnLayout { anchors.fill: parent; spacing: 0
                RowLayout { Layout.fillWidth: true; Layout.margins: 10; spacing: 6
                    Repeater {
                        model: [ {ic:"star",tx:"Favoritos"},{ic:"folder",tx:"Pastas"},{ic:"code",tx:"Snippets"},{ic:"key",tx:"Chaves"} ]
                        delegate: Rectangle {
                            required property var modelData; required property int index
                            Layout.fillWidth: true; implicitHeight: 52; radius: 9
                            color: win.sideMode === index ? cAccent : (mma.containsMouse ? "#23232A" : "transparent")
                            Behavior on color { ColorAnimation { duration: 120 } }
                            Column { anchors.centerIn: parent; spacing: 4
                                Icon { anchors.horizontalCenter: parent.horizontalCenter; width: 19; height: 19; kind: modelData.ic; stroke: win.sideMode === index ? "white" : cDim }
                                Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.tx; font.pixelSize: 10; color: win.sideMode === index ? "white" : cDim } }
                            MouseArea { id: mma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.sideMode = index }
                        }
                    }
                }
                Item { Layout.fillWidth: true; Layout.fillHeight: true
                    CrudList { anchors.fill: parent; visible: win.sideMode === 0; lm: favModel; title: "Favoritos"; iconKind: "star"; glyphColor: "#F0C24A"
                        onRowActivated: (i) => win.openTerminalAt(favModel.get(i).detail) }
                    ColumnLayout { anchors.fill: parent; visible: win.sideMode === 1; spacing: 0
                        RowLayout { Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 10; Layout.topMargin: 6; spacing: 2
                            Text { text: Files.baseName(Files.root); color: cText; font.pixelSize: 13; font.bold: true; elide: Text.ElideMiddle }
                            Item { Layout.fillWidth: true }
                            IconBtn { kind: "dots"; onClicked: dirMenu.open() }
                            IconBtn { kind: Files.showHidden ? "eye" : "eyeoff"; tint: Files.showHidden ? cAccent : cDim
                                tintHover: Files.showHidden ? cAccent : cText; onClicked: Files.toggleHidden() }
                            IconBtn { kind: "reload"; onClicked: win.reloadTree() } }
                        Text { text: Files.displayPath(Files.root); color: cDim; font.pixelSize: 12; Layout.leftMargin: 16; Layout.rightMargin: 10; Layout.topMargin: 2; elide: Text.ElideMiddle; Layout.fillWidth: true }
                        ListView { id: tree; Layout.fillWidth: true; Layout.fillHeight: true; Layout.topMargin: 6; clip: true; model: filesModel
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                            delegate: Rectangle {
                                required property int index; required property string fname; required property bool dir
                                required property bool open; required property int depth; required property string fpath
                                width: tree.width; height: 27
                                color: win.selFile === index ? "#243049" : (fma.containsMouse ? "#20202A" : "transparent")
                                Row { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 8 + depth * 15; spacing: 5
                                    Icon { width: 12; height: 12; kind: "chevron"; down: open; visible: dir; stroke: cDim; anchors.verticalCenter: parent.verticalCenter }
                                    Item { width: 12; height: 12; visible: !dir }
                                    Icon { width: 16; height: 16; kind: dir ? "folder" : "file"; stroke: dir ? "#5AA0F0" : cDim; anchors.verticalCenter: parent.verticalCenter }
                                    Text { text: fname; color: dir ? cText : cDim; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter } }
                                MouseArea { id: fma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                                    onClicked: (mouse) => {
                                        if (mouse.button === Qt.RightButton) {
                                            win.selFile = index; win.ctxRow = index; win.ctxName = fname; win.ctxPath = fpath; win.ctxDir = dir
                                            var p = mapToItem(Overlay.overlay, mouse.x, mouse.y)
                                            ctxMenu.x = p.x; ctxMenu.y = p.y; ctxMenu.open()
                                        } else win.toggleRow(index)
                                    } } } }
                    }
                    CrudList { anchors.fill: parent; visible: win.sideMode === 2; lm: snipModel; title: "Snippets"; iconKind: "code"; glyphColor: "#8FE08A"
                        onRowActivated: (i) => { if (win.focusedTerm) win.focusedTerm.send(snipModel.get(i).detail + "\r") } }
                    ColumnLayout { anchors.fill: parent; visible: win.sideMode === 3; spacing: 0
                        RowLayout { Layout.fillWidth: true; Layout.leftMargin: 14; Layout.rightMargin: 12; Layout.topMargin: 8; Layout.bottomMargin: 4
                            Text { text: "Credenciais"; color: cText; font.pixelSize: 13; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Rectangle { implicitWidth: kar.implicitWidth + 16; implicitHeight: 26; radius: 7; color: kam.containsMouse ? "#3576F5" : "#2D6BF0"
                                Row { id: kar; anchors.centerIn: parent; spacing: 4
                                    Text { text: "+"; color: "white"; font.pixelSize: 14; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                                    Text { text: "Nova"; color: "white"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter } }
                                MouseArea { id: kam; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.openCred(-1) } } }
                        ListView { Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: keyModel
                            delegate: Rectangle {
                                required property int index; required property string name; required property string type
                                width: ListView.view.width; height: 46; color: kma.containsMouse ? "#20202A" : "transparent"
                                MouseArea { id: kma; anchors.fill: parent; hoverEnabled: true }
                                RowLayout { anchors.fill: parent; anchors.leftMargin: 14; anchors.rightMargin: 8; spacing: 9
                                    Icon { Layout.alignment: Qt.AlignVCenter; width: 18; height: 18; kind: win.credIcon(type); stroke: "#5AA0F0" }
                                    ColumnLayout { spacing: 1; Layout.fillWidth: true
                                        Text { text: name; color: cText; font.pixelSize: 13; elide: Text.ElideRight; Layout.fillWidth: true }
                                        Text { text: type; color: cDim; font.pixelSize: 10 } }
                                    Rectangle { implicitWidth: 24; implicitHeight: 24; radius: 6; color: kedm.containsMouse ? "#2A2A36" : "transparent"
                                        Icon { anchors.centerIn: parent; width: 14; height: 14; kind: "pencil"; stroke: kedm.containsMouse ? cAccent : cDim }
                                        MouseArea { id: kedm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.openCred(index) } }
                                    Rectangle { implicitWidth: 24; implicitHeight: 24; radius: 6; color: kdlm.containsMouse ? "#43232A" : "transparent"
                                        Icon { anchors.centerIn: parent; width: 13; height: 13; kind: "x"; stroke: kdlm.containsMouse ? "#FF6A78" : cDim }
                                        MouseArea { id: kdlm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: keyModel.remove(index) } }
                                } } }
                    }
                }
            }
        }

        Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: cBorder }

        // ============ COLUNA PRINCIPAL ============
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
            Rectangle { Layout.fillWidth: true; height: 52; color: cPanel
                RowLayout { anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 12; spacing: 8
                    Text { text: "Workspace 1"; color: cText; font.pixelSize: 16; font.bold: true }
                    Text { text: win.splitMode ? (win.gridCols + "×" + win.gridRows + " · split") : (sessionsModel.count + (sessionsModel.count > 1 ? " abas" : " aba")); color: cDim; font.pixelSize: 12 }
                    Item { Layout.fillWidth: true }
                    Rectangle { id: resBtn; implicitWidth: resRow.implicitWidth + 14; implicitHeight: 26; radius: 7
                        color: resBtnMa.containsMouse ? "#23232E" : "transparent"
                        Row { id: resRow; anchors.centerIn: parent; spacing: 6
                            Pill { label: Math.round(Sys.cpu) + "% CPU"; fg: Sys.cpu > 80 ? "#E0563B" : "#E0A23B" }
                            Pill { label: Sys.memMB >= 1024 ? (Sys.memMB / 1024).toFixed(1) + "G" : Math.round(Sys.memMB) + "M"; fg: "#9AA0AA" } }
                        MouseArea { id: resBtnMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: resPopup.open() } }
                    Rectangle { id: micBtn; implicitWidth: 36; implicitHeight: 30; radius: 7
                        color: Voice.state === 1 ? "#3A1620" : (micMa.containsMouse ? "#2A2A34" : "transparent")
                        Behavior on color { ColorAnimation { duration: 110 } }
                        // anel pulsante enquanto grava
                        Rectangle { anchors.centerIn: parent; width: 26; height: 26; radius: 13; color: "transparent"
                            border.color: "#FF5A6E"; border.width: 1.5; visible: Voice.state === 1
                            SequentialAnimation on opacity { running: Voice.state === 1; loops: Animation.Infinite
                                NumberAnimation { from: 0.9; to: 0.2; duration: 700 } NumberAnimation { from: 0.2; to: 0.9; duration: 700 } } }
                        Icon { anchors.centerIn: parent; width: 16; height: 16; kind: "mic"
                            stroke: Voice.state === 1 ? "#FF5A6E" : (Voice.state === 2 ? "#E0A23B" : win.cText) }
                        MouseArea { id: micMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            enabled: Voice.state !== 2; onClicked: Voice.toggle() } }
                    TBtn { iconKind: "gear"; onClicked: settingsDlg.open() }
                    TBtn { iconKind: "trash"; label: "Limpar"; tint: "#E86A78"; onClicked: if (win.focusedTerm) win.focusedTerm.clearOutput() }
                    TBtn { iconKind: "split"; label: "Split Layout"; onClicked: splitPopup.open() }
                    TBtn { iconKind: "exit"; label: "Sair do Split"; tint: "#FF5A6E"; visible: win.splitMode; onClicked: win.splitMode = false }
                    Rectangle { implicitWidth: cbr.implicitWidth + 22; implicitHeight: 32; radius: 8
                        color: cbm.containsMouse ? "#2C2C3A" : "#23232E"; border.color: "#3A3550"; border.width: 1
                        Row { id: cbr; anchors.centerIn: parent; spacing: 6
                            Icon { width: 14; height: 14; kind: "spark"; stroke: "#B98BFF"; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: "Claude"; color: cText; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter } }
                        MouseArea { id: cbm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor } }
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: cBorder }

            // ===== TERMINAIS: barra de abas (SEMPRE visível) + área (single OU grid split) =====
            ColumnLayout {
                Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0

                // --- BARRA DE ABAS (sempre, inclusive no split) ---
                Rectangle { Layout.fillWidth: true; height: 38; color: cBg
                    RowLayout { anchors.fill: parent; anchors.leftMargin: 12; spacing: 6
                        Repeater { model: sessionsModel
                            delegate: Rectangle {
                                required property int index
                                implicitWidth: trow.implicitWidth + 22; implicitHeight: 30; radius: 8
                                color: win.curTerm === index ? "#1B2740" : (tbm.containsMouse ? "#1A1A20" : "transparent")
                                border.color: (win.dragTab === index) ? "#15C2D6" : (win.curTerm === index ? cAccent : "transparent"); border.width: 1
                                opacity: (win.dragTab === index) ? 0.5 : 1.0
                                MouseArea { id: tbm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                    property real px: 0; property bool moved: false
                                    onPressed: (m) => { px = m.x; moved = false }
                                    onPositionChanged: (m) => {
                                        if (!moved && sessionsModel.count > 1 && Math.abs(m.x - px) > 6) {
                                            moved = true; win.dragTab = index; dragGhost.label = "Terminal " + (index + 1); dragGhost.Drag.active = true
                                        }
                                        if (moved) { var p = mapToItem(dragLayer, m.x, m.y); dragGhost.x = p.x - dragGhost.width / 2; dragGhost.y = p.y - dragGhost.height / 2 }
                                    }
                                    onReleased: { if (moved) { dragGhost.Drag.active = false; win.dragTab = -1; moved = false } else { win.curTerm = index } }
                                }
                                DropArea { anchors.fill: parent; onEntered: win.reorderTab(index) }
                                Row { id: trow; anchors.centerIn: parent; spacing: 7
                                    Icon { width: 14; height: 14; kind: "terminal"; stroke: "#5AA0F0"; anchors.verticalCenter: parent.verticalCenter }
                                    Text { text: "Terminal " + (index + 1); color: win.curTerm === index ? cText : cDim; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter }
                                    Rectangle { width: 18; height: 18; radius: 4; color: xma.containsMouse ? "#43232A" : "transparent"; anchors.verticalCenter: parent.verticalCenter; visible: sessionsModel.count > 1
                                        Icon { anchors.centerIn: parent; width: 11; height: 11; kind: "x"; stroke: xma.containsMouse ? "#FF6A78" : cDim }
                                        MouseArea { id: xma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.closeTab(index) } }
                                }
                            }
                        }
                        TBtn { iconKind: "plus"; tint: cDim; onClicked: win.addTerm() }
                        Item { Layout.fillWidth: true }
                    }
                }

                Item {
                    Layout.fillWidth: true; Layout.fillHeight: true

                    // --- SINGLE (sem split) ---
                    TermView { anchors.fill: parent; anchors.margins: 10; visible: !win.splitMode
                        ctrl: terms.objectAt(Math.min(win.curTerm, sessionsModel.count - 1))
                        title: "Terminal " + (win.curTerm + 1) }

                    // --- GRID (split): slots fixos; vazio mantém o slot ---
                    GridLayout {
                        anchors.fill: parent; anchors.margins: 10; visible: win.splitMode
                        columns: win.gridCols; rowSpacing: 10; columnSpacing: 10
                        Repeater { model: slotModel
                            delegate: Item {
                                required property int index; required property int tid
                                Layout.fillWidth: true; Layout.fillHeight: true
                                TermView { anchors.fill: parent; visible: tid >= 0; slotIndex: index
                                    ctrl: win.ctrlForTid(tid); title: "Terminal " + (win.tabIndexForTid(tid) + 1)
                                    onCloseReq: win.closeSlot(index) }
                                Rectangle { anchors.fill: parent; visible: tid < 0; radius: 10; color: "#0B0B0F"
                                    border.width: 1.5; border.color: emptyMa.containsMouse ? cAccent : "#23232E"
                                    Column { anchors.centerIn: parent; spacing: 8
                                        Icon { anchors.horizontalCenter: parent.horizontalCenter; width: 28; height: 28; kind: "plus"; stroke: emptyMa.containsMouse ? cAccent : cDim }
                                        Text { anchors.horizontalCenter: parent.horizontalCenter; text: "Novo terminal"; color: emptyMa.containsMouse ? cText : cDim; font.pixelSize: 12 } }
                                    MouseArea { id: emptyMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.fillSlot(index) } }
                                DropArea { anchors.fill: parent; onDropped: if (win.dragSlot >= 0) win.swapSlots(win.dragSlot, index) }
                            } }
                    }
                }
            }
        }
    }

    // ===== popover Split Layout =====
    Popup {
        id: splitPopup
        parent: Overlay.overlay; anchors.centerIn: parent; width: 360; modal: true; padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: "#191920"; radius: 14; border.color: "#33333E"; border.width: 1 }
        contentItem: ColumnLayout { spacing: 14
            RowLayout { Layout.topMargin: 20; Layout.leftMargin: 22; Layout.rightMargin: 22
                Text { text: "Visão Dividida"; color: cText; font.pixelSize: 17; font.bold: true }
                Rectangle { implicitWidth: spl.implicitWidth + 14; implicitHeight: 20; radius: 6; color: "#1B3A6B"
                    Text { id: spl; anchors.centerIn: parent; text: "split ativo"; color: "#7FB0FF"; font.pixelSize: 10; font.bold: true } } }
            Text { text: "Presets"; color: cDim; font.pixelSize: 11; Layout.leftMargin: 22 }
            RowLayout { Layout.leftMargin: 22; Layout.rightMargin: 22; spacing: 10
                Repeater {
                    model: [ {c:2,r:1,l:"2×1"},{c:2,r:2,l:"2×2"},{c:3,r:2,l:"3×2"} ]
                    delegate: Rectangle { required property var modelData
                        Layout.fillWidth: true; implicitHeight: 44; radius: 9; color: ppm.containsMouse ? "#23314F" : "#1C1C24"; border.color: "#2E3850"; border.width: 1
                        Text { anchors.centerIn: parent; text: modelData.l + " · " + (modelData.c * modelData.r); color: cText; font.pixelSize: 12; font.bold: true }
                        MouseArea { id: ppm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { win.applyGrid(modelData.c, modelData.r); splitPopup.close() } } } }
            }
            RowLayout { Layout.leftMargin: 22; Layout.rightMargin: 22; Layout.topMargin: 4; spacing: 18
                Column { spacing: 5; Text { text: "Colunas"; color: cDim; font.pixelSize: 11 }
                    Stepper { value: win.gridCols; minv: 1; maxv: 4; onChanged: win.gridCols = v } }
                Column { spacing: 5; Text { text: "Linhas"; color: cDim; font.pixelSize: 11 }
                    Stepper { value: win.gridRows; minv: 1; maxv: 4; onChanged: win.gridRows = v } }
            }
            RowLayout { Layout.fillWidth: true; Layout.leftMargin: 22; Layout.rightMargin: 22; Layout.bottomMargin: 20; Layout.topMargin: 4
                Item { Layout.fillWidth: true }
                Rectangle { implicitWidth: 100; implicitHeight: 38; radius: 8; color: sfm.containsMouse ? "#3576F5" : cAccent
                    Text { anchors.centerIn: parent; text: "Aplicar"; color: "white"; font.pixelSize: 13; font.bold: true }
                    MouseArea { id: sfm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { win.applyGrid(win.gridCols, win.gridRows); splitPopup.close() } } }
            }
        }
    }

    // ===== popover Consumo de Recursos =====
    property var cpuHist: []
    Connections { target: Sys
        function onChanged() {
            var h = win.cpuHist.slice(); h.push(Sys.cpu)
            if (h.length > 40) h.shift()
            win.cpuHist = h
            if (resPopup.opened) spark.requestPaint()
        }
    }
    Popup {
        id: resPopup
        parent: Overlay.overlay; anchors.centerIn: parent; width: 340; modal: true; padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: "#191920"; radius: 14; border.color: "#33333E"; border.width: 1 }

        component Gauge: ColumnLayout { id: g
            property string title: ""; property string value: ""; property color tint: cAccent
            property real frac: 0   // 0..1
            Layout.fillWidth: true; spacing: 6
            RowLayout { Layout.fillWidth: true
                Text { text: g.title; color: cDim; font.pixelSize: 12 }
                Item { Layout.fillWidth: true }
                Text { text: g.value; color: g.tint; font.pixelSize: 15; font.bold: true } }
            Rectangle { Layout.fillWidth: true; implicitHeight: 8; radius: 4; color: "#26262E"
                Rectangle { width: Math.max(0, Math.min(1, g.frac)) * parent.width; height: parent.height; radius: 4; color: g.tint
                    Behavior on width { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } } } }
        }

        contentItem: ColumnLayout { spacing: 16
            RowLayout { Layout.topMargin: 20; Layout.leftMargin: 22; Layout.rightMargin: 22
                Text { text: "Consumo de Recursos"; color: cText; font.pixelSize: 17; font.bold: true }
                Item { Layout.fillWidth: true }
                Text { text: "● ao vivo"; color: "#4FD27B"; font.pixelSize: 10; font.bold: true } }

            Gauge { Layout.leftMargin: 22; Layout.rightMargin: 22
                title: "CPU"; tint: Sys.cpu > 80 ? "#E0563B" : "#E0A23B"
                value: Math.round(Sys.cpu) + "%"; frac: Sys.cpu / (Sys.cores * 100) }

            // mini-histórico de CPU (sparkline)
            Rectangle { Layout.leftMargin: 22; Layout.rightMargin: 22; Layout.fillWidth: true
                implicitHeight: 44; radius: 8; color: "#121218"; border.color: "#26262E"; border.width: 1
                Canvas { id: spark; anchors.fill: parent; anchors.margins: 6
                    onPaint: {
                        var ctx = getContext("2d"); ctx.reset()
                        var h = win.cpuHist; if (h.length < 2) return
                        var maxv = Sys.cores * 100
                        var w = width, ht = height
                        ctx.beginPath()
                        for (var i = 0; i < h.length; i++) {
                            var x = (i / (h.length - 1)) * w
                            var y = ht - Math.min(1, h[i] / maxv) * ht
                            if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y)
                        }
                        ctx.strokeStyle = "#5AA0F0"; ctx.lineWidth = 2; ctx.stroke()
                        ctx.lineTo(w, ht); ctx.lineTo(0, ht); ctx.closePath()
                        ctx.fillStyle = "#1A2E50"; ctx.globalAlpha = 0.5; ctx.fill()
                    }
                } }

            Gauge { Layout.leftMargin: 22; Layout.rightMargin: 22
                title: "Memória"; tint: "#9AA0AA"
                value: Sys.memMB >= 1024 ? (Sys.memMB / 1024).toFixed(2) + " GB" : Math.round(Sys.memMB) + " MB"
                frac: Sys.memMB / 2048 }

            Rectangle { Layout.leftMargin: 22; Layout.rightMargin: 22; Layout.fillWidth: true; height: 1; color: "#26262E" }

            RowLayout { Layout.leftMargin: 22; Layout.rightMargin: 22; spacing: 0
                Column { spacing: 2
                    Text { text: "Terminais"; color: cDim; font.pixelSize: 11 }
                    Text { text: sessionsModel.count; color: cText; font.pixelSize: 16; font.bold: true } }
                Item { Layout.fillWidth: true }
                Column { spacing: 2
                    Text { text: "Núcleos"; color: cDim; font.pixelSize: 11; horizontalAlignment: Text.AlignRight; width: parent.width }
                    Text { text: Sys.cores; color: cText; font.pixelSize: 16; font.bold: true; horizontalAlignment: Text.AlignRight; width: parent.width } } }

            Text { Layout.leftMargin: 22; Layout.rightMargin: 22; Layout.bottomMargin: 20; Layout.fillWidth: true
                text: "100% = 1 núcleo · topo do gráfico = " + Sys.cores + " núcleos"
                color: "#5A5A64"; font.pixelSize: 10; wrapMode: Text.WordWrap }
        }
    }

    // ===== camada do "fantasma" de arrasto (reorder de terminais) =====
    Item { id: dragLayer; anchors.fill: parent; z: 9000
        Rectangle { id: dragGhost; visible: win.dragTab >= 0 || win.dragSlot >= 0; width: 168; height: 38; radius: 9
            color: "#2E7BF0"; opacity: 0.92; border.color: "#7FB0FF"; border.width: 1
            property string label: ""
            Drag.active: false; Drag.hotSpot.x: width / 2; Drag.hotSpot.y: height / 2
            Row { anchors.centerIn: parent; spacing: 7
                Icon { width: 14; height: 14; kind: "terminal"; stroke: "white"; anchors.verticalCenter: parent.verticalCenter }
                Text { text: dragGhost.label; color: "white"; font.pixelSize: 12; font.bold: true; anchors.verticalCenter: parent.verticalCenter } } }
    }

    // ===== mic (Voice/Groq): roteia a transcrição + mostra status/erro =====
    Connections { target: Voice
        function onTranscribed(t) { if (win.focusedTerm) win.focusedTerm.requestInsert(t) }
        function onChanged() {
            if (Voice.state === 1) { toast.txt = "Gravando… clique no mic para parar"; toast.err = false; toast.show = true; toastTimer.stop() }
            else if (Voice.state === 2) { toast.txt = "Transcrevendo…"; toast.err = false; toast.show = true; toastTimer.stop() }
            else if (Voice.error.length > 0) { toast.txt = Voice.error; toast.err = true; toast.show = true; toastTimer.restart() }
            else { toastTimer.restart() }
        }
    }
    Timer { id: toastTimer; interval: 3500; onTriggered: toast.show = false }
    Rectangle { id: toast; property bool show: false; property bool err: false; property string txt: ""
        z: 9500; visible: show; opacity: show ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 180 } }
        anchors.bottom: parent.bottom; anchors.bottomMargin: 28; anchors.horizontalCenter: parent.horizontalCenter
        implicitWidth: toastRow.implicitWidth + 28; implicitHeight: 40; radius: 10
        color: err ? "#3A1620" : "#15233F"; border.color: err ? "#FF5A6E" : "#2D6BF0"; border.width: 1
        Row { id: toastRow; anchors.centerIn: parent; spacing: 9
            Icon { width: 15; height: 15; kind: toast.err ? "x" : "mic"; stroke: toast.err ? "#FF5A6E" : "#7FB0FF"; anchors.verticalCenter: parent.verticalCenter }
            Text { text: toast.txt; color: cText; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter } } }

    // ===== menu de contexto da árvore de pastas (botão direito) =====
    Popup {
        id: ctxMenu; parent: Overlay.overlay; padding: 6; width: 244
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: "#1C1C22"; radius: 11; border.color: "#34343E"; border.width: 1 }
        contentItem: ColumnLayout { spacing: 1
            Repeater {
                model: [
                    { a: "expand", t: "Expandir / Colapsar", dirOnly: true },
                    { a: "terminal", t: "Abrir terminal aqui" },
                    { a: "root", t: "Abrir como raiz", dirOnly: true },
                    { a: "fav", t: "Adicionar aos Favoritos" },
                    { sep: true },
                    { a: "copy", t: "Copiar caminho" },
                    { a: "reveal", t: "Revelar no sistema" },
                    { sep: true },
                    { a: "rename", t: "Renomear" },
                    { a: "newfile", t: "Novo arquivo aqui" },
                    { a: "newfolder", t: "Nova pasta aqui" },
                    { sep: true },
                    { a: "trash", t: "Mover pra Lixeira", danger: true }
                ]
                delegate: Item {
                    required property var modelData
                    Layout.fillWidth: true
                    visible: !(modelData.dirOnly === true && !win.ctxDir)
                    implicitHeight: !visible ? 0 : (modelData.sep === true ? 9 : 34)
                    Rectangle { visible: modelData.sep === true; anchors.verticalCenter: parent.verticalCenter; x: 8; width: parent.width - 16; height: 1; color: "#2E2E38" }
                    Rectangle { visible: modelData.sep !== true; anchors.fill: parent; radius: 7; color: cim.containsMouse ? "#2A2A36" : "transparent"
                        Text { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12; text: modelData.t || ""; color: modelData.danger === true ? "#FF6A78" : cText; font.pixelSize: 13 }
                        MouseArea { id: cim; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.ctxAction(modelData.a) } }
                }
            }
        }
    }

    // ===== diálogo de nome (renomear / novo arquivo / nova pasta) =====
    Popup {
        id: nameDlg; parent: Overlay.overlay; anchors.centerIn: parent; width: 360; modal: true; padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: "#191920"; radius: 14; border.color: "#33333E"; border.width: 1 }
        contentItem: ColumnLayout { spacing: 14
            Text { text: win.nameMode === "rename" ? "Renomear" : (win.nameMode === "newfolder" ? "Nova pasta" : "Novo arquivo"); color: cText; font.pixelSize: 16; font.bold: true; Layout.topMargin: 18; Layout.leftMargin: 20 }
            FField { id: nameField; Layout.fillWidth: true; Layout.leftMargin: 20; Layout.rightMargin: 20; placeholder: "nome" }
            RowLayout { Layout.fillWidth: true; Layout.leftMargin: 20; Layout.rightMargin: 20; Layout.bottomMargin: 18; spacing: 10
                Item { Layout.fillWidth: true }
                Rectangle { implicitWidth: 96; implicitHeight: 36; radius: 8; color: ncm.containsMouse ? "#23232E" : "transparent"; border.color: "#33333E"; border.width: 1
                    Text { anchors.centerIn: parent; text: "Cancelar"; color: cText; font.pixelSize: 13 }
                    MouseArea { id: ncm; anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: nameDlg.close() } }
                Rectangle { implicitWidth: 96; implicitHeight: 36; radius: 8; color: nok.containsMouse ? "#3576F5" : cAccent
                    Text { anchors.centerIn: parent; text: "OK"; color: "white"; font.pixelSize: 13; font.bold: true }
                    MouseArea { id: nok; anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: win.nameConfirm() } }
            }
        }
    }

    // ===== diálogo de Configurações (preferências) =====
    Popup {
        id: settingsDlg; parent: Overlay.overlay; anchors.centerIn: parent; width: 470; modal: true; padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: "#191920"; radius: 16; border.color: "#33333E"; border.width: 1 }
        onOpened: { groqKeyField.text = Settings.groqApiKey; groqModelField.text = Settings.groqModel; shellField.text = Settings.defaultShell }
        contentItem: ColumnLayout { spacing: 14
            RowLayout { Layout.topMargin: 20; Layout.leftMargin: 22; Layout.rightMargin: 22; spacing: 9
                Icon { Layout.alignment: Qt.AlignVCenter; width: 19; height: 19; kind: "gear"; stroke: cText }
                Text { text: "Configurações"; color: cText; font.pixelSize: 18; font.bold: true } }
            Text { text: "TRANSCRIÇÃO DE VOZ (MIC)"; color: cDim; font.pixelSize: 10; font.bold: true; Layout.leftMargin: 22 }
            Column { Layout.fillWidth: true; Layout.leftMargin: 22; Layout.rightMargin: 22; spacing: 5
                Text { text: "Groq API Key — groq.com (Whisper)"; color: cDim; font.pixelSize: 11 }
                FField { id: groqKeyField; width: parent.width; placeholder: "gsk_..."; secret: true } }
            Column { Layout.fillWidth: true; Layout.leftMargin: 22; Layout.rightMargin: 22; spacing: 5
                Text { text: "Modelo Whisper"; color: cDim; font.pixelSize: 11 }
                FField { id: groqModelField; width: parent.width; placeholder: "whisper-large-v3" } }
            Rectangle { Layout.leftMargin: 22; Layout.rightMargin: 22; Layout.fillWidth: true; height: 1; color: "#26262E" }
            Text { text: "TERMINAL"; color: cDim; font.pixelSize: 10; font.bold: true; Layout.leftMargin: 22 }
            Column { Layout.fillWidth: true; Layout.leftMargin: 22; Layout.rightMargin: 22; spacing: 5
                Text { text: "Shell padrão (vazio = padrão do sistema)"; color: cDim; font.pixelSize: 11 }
                FField { id: shellField; width: parent.width; placeholder: "powershell.exe / bash" } }
            RowLayout { Layout.fillWidth: true; Layout.leftMargin: 22; Layout.rightMargin: 22; Layout.bottomMargin: 20; Layout.topMargin: 4; spacing: 10
                Item { Layout.fillWidth: true }
                Rectangle { implicitWidth: 96; implicitHeight: 38; radius: 8; color: scm.containsMouse ? "#23232E" : "transparent"; border.color: "#33333E"; border.width: 1
                    Text { anchors.centerIn: parent; text: "Fechar"; color: cText; font.pixelSize: 13 }
                    MouseArea { id: scm; anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: settingsDlg.close() } }
                Rectangle { implicitWidth: 112; implicitHeight: 38; radius: 8; color: ssm.containsMouse ? "#3576F5" : cAccent
                    Text { anchors.centerIn: parent; text: "Salvar"; color: "white"; font.pixelSize: 13; font.bold: true }
                    MouseArea { id: ssm; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: { Settings.groqApiKey = groqKeyField.text; Settings.groqModel = groqModelField.text; Settings.defaultShell = shellField.text; settingsDlg.close() } } }
            }
        }
    }

    // ===== menu de atalhos do sistema (•••) =====
    FolderDialog { id: folderDlg; title: "Escolher pasta"
        onAccepted: Files.root = Files.fromUrl(selectedFolder) }
    Popup {
        id: dirMenu
        parent: Overlay.overlay; modal: true; padding: 6; width: 230
        x: win.width / 2 - 200; y: 92
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: "#1C1C22"; radius: 11; border.color: "#34343E"; border.width: 1 }

        component MItem: Rectangle {
            property string label: ""; property bool danger: false
            signal act()
            Layout.fillWidth: true; implicitHeight: 34; radius: 7
            color: mim.containsMouse ? "#2A2A36" : "transparent"
            Text { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12
                text: parent.label; color: parent.danger ? "#E0A23B" : cText; font.pixelSize: 13 }
            MouseArea { id: mim; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                onClicked: { parent.act(); dirMenu.close() } }
        }
        component MSep: Rectangle { Layout.fillWidth: true; Layout.leftMargin: 8; Layout.rightMargin: 8; implicitHeight: 1; color: "#2E2E38" }

        contentItem: ColumnLayout { spacing: 1
            MItem { label: "Home";      onAct: Files.root = Files.home() }
            MItem { label: "Desktop";   onAct: Files.root = Files.standard("desktop") }
            MItem { label: "Documents"; onAct: Files.root = Files.standard("documents") }
            MItem { label: "Downloads"; onAct: Files.root = Files.standard("downloads") }
            MSep {}
            MItem { label: "Escolher pasta…"; onAct: folderDlg.open() }
            MSep {}
            MItem { label: Files.showHidden ? "Ocultar arquivos ocultos" : "Mostrar arquivos ocultos"
                danger: true; onAct: Files.toggleHidden() }
        }
    }

    // ===== diálogo simples (fav/snippets) =====
    Popup {
        id: itemDialog
        parent: Overlay.overlay; anchors.centerIn: parent; width: 420; modal: true; padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onOpened: fTitle.forceActiveFocus()
        background: Rectangle { color: "#191920"; radius: 14; border.color: "#33333E"; border.width: 1 }
        contentItem: ColumnLayout { spacing: 14
            Text { id: dlgTitleLbl; text: "Adicionar"; color: cText; font.pixelSize: 16; font.bold: true; Layout.topMargin: 20; Layout.leftMargin: 20 }
            Column { Layout.fillWidth: true; Layout.leftMargin: 20; Layout.rightMargin: 20; spacing: 5
                Text { text: "Título"; color: cDim; font.pixelSize: 11 }
                FField { id: fTitle; width: parent.width; placeholder: "ex.: Servidor de produção" } }
            Column { Layout.fillWidth: true; Layout.leftMargin: 20; Layout.rightMargin: 20; spacing: 5
                Text { text: "Detalhe / valor"; color: cDim; font.pixelSize: 11 }
                FField { id: fDetail; width: parent.width; placeholder: "ex.: root@1.2.3.4  ·  comando  ·  valor" } }
            RowLayout { Layout.fillWidth: true; Layout.leftMargin: 20; Layout.rightMargin: 20; Layout.bottomMargin: 20
                Item { Layout.fillWidth: true }
                Rectangle { implicitWidth: 96; implicitHeight: 36; radius: 8; color: cnm.containsMouse ? "#2A2A34" : "#23232C"
                    Text { anchors.centerIn: parent; text: "Cancelar"; color: cText; font.pixelSize: 13 }
                    MouseArea { id: cnm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: itemDialog.close() } }
                Rectangle { implicitWidth: 96; implicitHeight: 36; radius: 8
                    color: fTitle.text.length === 0 ? "#244089" : (svm.containsMouse ? "#3576F5" : cAccent)
                    Text { anchors.centerIn: parent; text: "Salvar"; color: "white"; font.pixelSize: 13; font.bold: true }
                    MouseArea { id: svm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: {
                        if (fTitle.text.trim().length === 0) return
                        if (win.dlgIndex < 0) win.dlgModel.append({ title: fTitle.text, detail: fDetail.text })
                        else { win.dlgModel.setProperty(win.dlgIndex, "title", fTitle.text); win.dlgModel.setProperty(win.dlgIndex, "detail", fDetail.text) }
                        itemDialog.close() } } }
            }
        }
    }

    // ===== diálogo de CREDENCIAL =====
    Popup {
        id: credDialog
        parent: Overlay.overlay; anchors.centerIn: parent; width: 560; modal: true; padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onOpened: credNameField.forceActiveFocus()
        background: Rectangle { color: "#191920"; radius: 16; border.color: "#33333E"; border.width: 1 }
        contentItem: ColumnLayout { spacing: 12
            Text { text: win.credIndex < 0 ? "Nova Credencial" : "Editar Credencial"; color: cText; font.pixelSize: 18; font.bold: true; Layout.topMargin: 20; Layout.leftMargin: 22 }
            RowLayout { Layout.fillWidth: true; Layout.leftMargin: 22; Layout.rightMargin: 22; spacing: 14
                Column { Layout.fillWidth: true; spacing: 5
                    Text { text: "Nome"; color: cDim; font.pixelSize: 11 }
                    FField { id: credNameField; width: parent.width; placeholder: "Ex: Cliente A — VPS Produção" } }
                Column { spacing: 5
                    Text { text: "Tipo"; color: cDim; font.pixelSize: 11 }
                    Rectangle { id: credTypeSel; property string value: "SSH"
                        width: 150; implicitHeight: 38; radius: 8; color: cField; border.color: "#33333E"; border.width: 1
                        Row { anchors.left: parent.left; anchors.leftMargin: 10; anchors.verticalCenter: parent.verticalCenter; spacing: 6
                            Icon { width: 15; height: 15; kind: win.credIcon(credTypeSel.value); stroke: "#5AA0F0"; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: credTypeSel.value; color: cText; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter } }
                        Text { text: "▾"; color: cDim; font.pixelSize: 12; anchors.right: parent.right; anchors.rightMargin: 10; anchors.verticalCenter: parent.verticalCenter }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: typePopup.open() }
                        Popup { id: typePopup; y: parent.height + 4; width: parent.width; padding: 4
                            background: Rectangle { color: "#1C1C24"; radius: 8; border.color: "#33333E" }
                            contentItem: Column { spacing: 2
                                Repeater { model: win.credTypes
                                    delegate: Rectangle { required property var modelData; width: typePopup.width - 8; height: 30; radius: 6; color: tpm.containsMouse ? "#2A2A36" : "transparent"
                                        Text { anchors.left: parent.left; anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter; text: modelData; color: cText; font.pixelSize: 13 }
                                        MouseArea { id: tpm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { win.applyType(modelData); typePopup.close() } } } } } } }
                }
            }
            RowLayout { Layout.fillWidth: true; Layout.leftMargin: 22; Layout.rightMargin: 22
                Text { text: "Campos"; color: cDim; font.pixelSize: 11; font.bold: true }
                Item { Layout.fillWidth: true }
                Text { text: "+ Campo"; color: cAccent; font.pixelSize: 12; font.bold: true
                    MouseArea { anchors.fill: parent; anchors.margins: -6; cursorShape: Qt.PointingHandCursor; onClicked: dlgFields.append({ key: "", value: "", secret: false, revealed: false }) } }
            }
            Column { Layout.fillWidth: true; Layout.leftMargin: 22; Layout.rightMargin: 22; spacing: 6
                Repeater { model: dlgFields
                    delegate: RowLayout { id: fieldRow
                        required property int index; required property string key; required property string value; required property bool secret; required property bool revealed
                        width: parent.width; spacing: 6
                        FField { implicitWidth: 120; placeholder: "campo"; Component.onCompleted: text = fieldRow.key; onTextChanged: dlgFields.setProperty(fieldRow.index, "key", text) }
                        FField { Layout.fillWidth: true; placeholder: "valor"; secret: fieldRow.secret && !fieldRow.revealed; Component.onCompleted: text = fieldRow.value; onTextChanged: dlgFields.setProperty(fieldRow.index, "value", text) }
                        Rectangle { implicitWidth: 30; implicitHeight: 30; radius: 6; color: evm.containsMouse ? "#2A2A36" : "transparent"; visible: fieldRow.secret
                            Icon { anchors.centerIn: parent; width: 15; height: 15; kind: fieldRow.revealed ? "eye" : "eyeoff"; stroke: evm.containsMouse ? cAccent : cDim }
                            MouseArea { id: evm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: dlgFields.setProperty(fieldRow.index, "revealed", !fieldRow.revealed) } }
                        Rectangle { implicitWidth: 30; implicitHeight: 30; radius: 6; color: rmm.containsMouse ? "#43232A" : "transparent"
                            Icon { anchors.centerIn: parent; width: 14; height: 14; kind: "x"; stroke: rmm.containsMouse ? "#FF6A78" : cDim }
                            MouseArea { id: rmm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: dlgFields.remove(fieldRow.index) } }
                    } }
            }
            Column { Layout.fillWidth: true; Layout.leftMargin: 22; Layout.rightMargin: 22; spacing: 5
                Text { text: "Notas (opcional)"; color: cDim; font.pixelSize: 11 }
                Rectangle { width: parent.width; height: 56; radius: 8; color: cField; border.color: "#33333E"; border.width: 1
                    TextArea { id: credNotesField; anchors.fill: parent; anchors.margins: 6; color: cText; font.pixelSize: 13; wrapMode: TextArea.Wrap; background: Item {} } }
            }
            RowLayout { Layout.fillWidth: true; Layout.leftMargin: 22; Layout.rightMargin: 22; Layout.bottomMargin: 20; Layout.topMargin: 4
                Item { Layout.fillWidth: true }
                Rectangle { implicitWidth: 100; implicitHeight: 38; radius: 8; color: ccm.containsMouse ? "#2A2A34" : "#23232C"
                    Text { anchors.centerIn: parent; text: "Cancelar"; color: cText; font.pixelSize: 13 }
                    MouseArea { id: ccm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: credDialog.close() } }
                Rectangle { implicitWidth: 110; implicitHeight: 38; radius: 8
                    color: credNameField.text.length === 0 ? "#244089" : (camm.containsMouse ? "#3576F5" : cAccent)
                    Text { anchors.centerIn: parent; text: win.credIndex < 0 ? "Adicionar" : "Salvar"; color: "white"; font.pixelSize: 13; font.bold: true }
                    MouseArea { id: camm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.saveCred() } }
            }
        }
    }
}
