(function () {
  'use strict';

  var COLUMNS = [
    ['waiting', 'Waiting'],
    ['todo', 'To do'],
    ['assigned', 'Assigned'],
    ['progress', 'In progress'],
    ['review', 'In review'],
    ['done', 'Done']
  ];

  var COLUMN_OF = {
    Created: 'waiting',
    Available: 'todo',
    Assigned: 'assigned',
    Rejected: 'assigned',
    Escalated: 'assigned',
    InProgress: 'progress',
    AwaitingApproval: 'review',
    Completed: 'done',
    Cancelled: 'done'
  };

  var STATUS_LABEL = {
    Created: 'Waiting',
    Available: 'To do',
    Assigned: 'Assigned',
    Rejected: 'Changes requested',
    Escalated: 'Escalated',
    InProgress: 'In progress',
    AwaitingApproval: 'In review',
    Completed: 'Done',
    Cancelled: 'Cancelled'
  };

  var PROJECT_STATE = {
    Running: ['accent', 'Running'],
    AwaitingSignOff: ['amber', 'Waiting for owner sign-off'],
    Completed: ['green', 'Completed'],
    Cancelled: ['', 'Cancelled'],
    Created: ['', 'Created']
  };

  var ACTION_LABEL = {
    claim: 'Take it',
    begin: 'Start',
    done: 'Send for approval',
    withdraw: 'Take back from review',
    approve: 'Approve',
    reject: 'Request changes',
    escalate: 'Escalate',
    autoAssign: 'Auto-assign'
  };

  var MOVE_LABEL = {
    claim: 'Take it',
    claimStart: 'Take it and start',
    pickAssignee: 'Choose who does it',
    begin: 'Start',
    done: 'Send for approval',
    withdraw: 'Take back from review',
    approve: 'Approve',
    reject: 'Request changes'
  };

  var PRIORITY_CLASS = { Low: '', Medium: 'blue', High: 'amber', Urgent: 'red' };
  var PRIORITY_RANK = { Low: 0, Medium: 1, High: 2, Urgent: 3 };

  var POLICY_HINT = {
    SmallTeam: 'Stages run one item at a time, work goes to the first person with the role, simple approvals',
    BusyTeam: 'Items in a stage run in parallel, work goes to whoever has the least on their plate',
    HighRisk: 'Strict order, reviews go to the tech lead, every approval needs a person'
  };

  var ICON = {
    close: '<svg viewBox="0 0 16 16" width="16" height="16" aria-hidden="true"><path d="M4 4 L12 12 M12 4 L4 12" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"/></svg>',
    menu: '<svg viewBox="0 0 16 16" width="18" height="18" aria-hidden="true"><path d="M2.5 4.5 H13.5 M2.5 8 H13.5 M2.5 11.5 H13.5" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"/></svg>',
    chevron: '<svg viewBox="0 0 16 16" width="12" height="12" aria-hidden="true"><path d="M6 4 L10 8 L6 12" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round"/></svg>',
    plus: '<svg viewBox="0 0 16 16" width="14" height="14" aria-hidden="true"><path d="M8 3 V13 M3 8 H13" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"/></svg>',
    spark: '<svg viewBox="0 0 16 16" width="14" height="14" aria-hidden="true"><path d="M8 2 L9.4 6.6 L14 8 L9.4 9.4 L8 14 L6.6 9.4 L2 8 L6.6 6.6 Z" fill="currentColor"/></svg>'
  };

  var store = {
    ws: null,
    connected: false,
    retry: 500,
    hello: null,
    view: null,
    page: read('page') || 'mywork',
    project: read('project'),
    selected: null,
    modal: null,
    form: {},
    picker: null,
    rejectOpen: false,
    subOpen: false,
    navOpen: false,
    openFrom: null,
    collapsed: readCollapsed(),
    dirty: false,
    ref: 1,
    pending: {}
  };

  var drag = null;
  var suppressClick = false;

  function read(key) {
    try { return window.localStorage.getItem('tividy.' + key); } catch (e) { return null; }
  }

  function write(key, value) {
    try {
      if (value === null || value === undefined) window.localStorage.removeItem('tividy.' + key);
      else window.localStorage.setItem('tividy.' + key, value);
    } catch (e) { }
  }

  function readCollapsed() {
    try { return JSON.parse(read('collapsed') || '{"waiting":true}'); } catch (e) { return { waiting: true }; }
  }

  function esc(value) {
    return String(value === undefined || value === null ? '' : value)
      .replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;').replace(/'/g, '&#39;');
  }

  function hue(name) {
    var h = 0;
    for (var i = 0; i < name.length; i++) h = (h * 31 + name.charCodeAt(i)) % 360;
    return h;
  }

  function avatar(name, size) {
    if (!name) return '<span class="avatar empty' + (size ? ' ' + size : '') + '" title="Unassigned">?</span>';
    var initials = name.split(/\s+/).map(function (p) { return p.charAt(0); }).join('').slice(0, 2).toUpperCase();
    return '<span class="avatar' + (size ? ' ' + size : '') + '" title="' + esc(name) +
      '" style="background:hsl(' + hue(name) + ',52%,50%)">' + esc(initials) + '</span>';
  }

  function priorityChip(priority) {
    var cls = PRIORITY_CLASS[priority];
    return '<span class="chip prio ' + (cls || '') + '" title="Priority">' + priorityBars(priority) + esc(priority) + '</span>';
  }

  function priorityBars(priority) {
    var rank = PRIORITY_RANK[priority] || 0;
    var bars = '';
    for (var i = 0; i < 3; i++) {
      var on = rank === 3 || i < rank;
      bars += '<rect x="' + (1 + i * 4) + '" y="' + (8 - i * 3) + '" width="3" height="' + (3 + i * 3) + '" rx="0.5" fill="currentColor" opacity="' + (on ? 1 : 0.25) + '"/>';
    }
    return '<svg width="13" height="12" viewBox="0 0 13 12">' + bars + '</svg>';
  }

  function statusIcon(status) {
    var c = 'var(--faint)';
    var inner = '';
    var ring = '<circle cx="7" cy="7" r="5.5" fill="none" stroke="' + c + '" stroke-width="1.5"/>';
    if (status === 'Created') {
      ring = '<circle cx="7" cy="7" r="5.5" fill="none" stroke="' + c + '" stroke-width="1.5" stroke-dasharray="2 2"/>';
    } else if (status === 'Assigned') {
      c = 'var(--blue)';
      ring = '<circle cx="7" cy="7" r="5.5" fill="none" stroke="' + c + '" stroke-width="1.5"/>';
      inner = '<circle cx="7" cy="7" r="1.6" fill="' + c + '"/>';
    } else if (status === 'InProgress') {
      c = 'var(--amber)';
      ring = '<circle cx="7" cy="7" r="5.5" fill="none" stroke="' + c + '" stroke-width="1.5"/>';
      inner = '<path d="M7 3.5 A3.5 3.5 0 0 1 7 10.5 Z" fill="' + c + '"/>';
    } else if (status === 'AwaitingApproval' || status === 'AwaitingSignOff') {
      c = 'var(--accent)';
      ring = '<circle cx="7" cy="7" r="5.5" fill="none" stroke="' + c + '" stroke-width="1.5"/>';
      inner = '<path d="M7 3.5 A3.5 3.5 0 1 1 3.5 7 L7 7 Z" fill="' + c + '"/>';
    } else if (status === 'Completed') {
      c = 'var(--green)';
      ring = '<circle cx="7" cy="7" r="6" fill="' + c + '"/>';
      inner = '<path d="M4.3 7.2 L6.2 9 L9.8 5.2" fill="none" stroke="#fff" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round"/>';
    } else if (status === 'Rejected') {
      c = 'var(--red)';
      ring = '<circle cx="7" cy="7" r="6" fill="' + c + '"/>';
      inner = '<path d="M5 5 L9 9 M9 5 L5 9" stroke="#fff" stroke-width="1.5" stroke-linecap="round"/>';
    } else if (status === 'Escalated') {
      c = 'var(--amber)';
      ring = '<circle cx="7" cy="7" r="6" fill="' + c + '"/>';
      inner = '<path d="M7 4 L7 7.6" stroke="#fff" stroke-width="1.6" stroke-linecap="round"/><circle cx="7" cy="9.8" r="0.9" fill="#fff"/>';
    } else if (status === 'Cancelled') {
      ring = '<circle cx="7" cy="7" r="5.5" fill="none" stroke="' + c + '" stroke-width="1.5"/>';
      inner = '<path d="M4 10 L10 4" stroke="' + c + '" stroke-width="1.5"/>';
    }
    return '<svg class="status" viewBox="0 0 14 14" role="img" aria-label="' + esc(STATUS_LABEL[status] || status) + '">' + ring + inner + '</svg>';
  }

  function progressRing(value) {
    var r = 5.5;
    var len = 2 * Math.PI * r;
    return '<svg class="ring" viewBox="0 0 14 14"><circle cx="7" cy="7" r="' + r + '" fill="none" stroke="var(--line-strong)" stroke-width="2"/>' +
      '<circle cx="7" cy="7" r="' + r + '" fill="none" stroke="var(--accent)" stroke-width="2" stroke-dasharray="' +
      (len * value).toFixed(2) + ' ' + len.toFixed(2) + '" transform="rotate(-90 7 7)"/></svg>';
  }

  function connect() {
    var scheme = window.location.protocol === 'https:' ? 'wss://' : 'ws://';
    var ws = new WebSocket(scheme + window.location.host + '/ws');
    store.ws = ws;
    ws.onopen = function () {
      store.connected = true;
      store.retry = 500;
      render();
    };
    ws.onmessage = function (e) {
      var msg;
      try { msg = JSON.parse(e.data); } catch (err) { return; }
      receive(msg);
    };
    ws.onclose = function () {
      store.connected = false;
      Object.keys(store.pending).forEach(function (ref) {
        store.pending[ref]({ ok: false, error: 'Connection lost' });
      });
      store.pending = {};
      render();
      setTimeout(connect, store.retry);
      store.retry = Math.min(store.retry * 2, 5000);
    };
  }

  function send(cmd, args) {
    return new Promise(function (resolve) {
      if (!store.ws || store.ws.readyState !== 1) {
        resolve({ ok: false, error: 'Not connected' });
        return;
      }
      var ref = store.ref++;
      store.pending[ref] = resolve;
      store.ws.send(JSON.stringify({ cmd: cmd, args: args || {}, ref: ref }));
    });
  }

  var CONFIRM = { claim: 1, begin: 1, done: 1, withdraw: 1, approve: 1, reject: 1, assign: 1, autoAssign: 1, escalate: 1 };

  function run(cmd, args) {
    return send(cmd, args).then(function (result) {
      if (!result.ok) {
        toast(result.error || 'Something went wrong', 'error');
        render();
      } else {
        clearErrors();
        if (CONFIRM[cmd] && args && args.item) confirmItem(args.project, args.item);
      }
      return result;
    });
  }

  function clearErrors() {
    var errors = document.querySelectorAll('#toasts .toast.error');
    for (var i = 0; i < errors.length; i++) errors[i].parentNode.removeChild(errors[i]);
  }

  function confirmItem(projectId, itemId) {
    setTimeout(function () {
      var item = findItem(projectId, itemId);
      if (!item) return;
      var text = item.name + ': ' + (STATUS_LABEL[item.status] || item.status);
      if (item.assignee && (item.status === 'Assigned' || item.status === 'InProgress')) text += ', ' + item.assignee;
      if (item.status === 'AwaitingApproval' && item.pendingApprovers.length) text += ', waiting for ' + item.pendingApprovers.join(' and ');
      toast(text, 'ok', store.view && store.view.history.canUndo);
    }, 90);
  }

  function receive(msg) {
    if (msg.type === 'result') {
      var done = store.pending[msg.ref];
      if (done) {
        delete store.pending[msg.ref];
        done(msg);
      }
      return;
    }
    if (msg.type === 'hello') {
      store.hello = msg;
      store.view = null;
      store.selected = null;
      store.picker = null;
      var saved = read('person');
      if (saved && msg.people.some(function (p) { return p.name === saved; })) {
        send('login', { as: saved }).then(function (r) { if (!r.ok) write('person', null); render(); });
      }
      render();
      return;
    }
    if (msg.type === 'state') {
      store.view = msg;
      if (store.project && !findProject(store.project)) store.project = null;
      if (!store.project && msg.projects.length) store.project = msg.projects[msg.projects.length - 1].id;
      if (store.selected && !findItem(store.selected.project, store.selected.item)) closeDialog();
      if (store.picker && !canAssign(selectedItem())) store.picker = null;
      if (store.rejectOpen && !hasAction(selectedItem(), 'reject')) store.rejectOpen = false;
      render();
      return;
    }
    if (msg.type === 'event') {
      var broad = /^Instance|ItemAdded|OrgChanged|IssueFlagged|Undone|Redone/.test(msg.event);
      if (msg.mine || broad) toast(msg.text, msg.mine ? 'mine' : '');
    }
  }

  function me() { return store.view ? store.view.me : null; }
  function isManager() { return !!(me() && me().manager); }

  function findProject(id) {
    if (!store.view) return null;
    for (var i = 0; i < store.view.projects.length; i++) {
      if (store.view.projects[i].id === id) return store.view.projects[i];
    }
    return null;
  }

  function findItem(projectId, itemId) {
    var p = findProject(projectId);
    if (!p) return null;
    for (var i = 0; i < p.items.length; i++) if (p.items[i].id === itemId) return p.items[i];
    return null;
  }

  function selectedItem() {
    return store.selected ? findItem(store.selected.project, store.selected.item) : null;
  }

  function hasAction(item, action) {
    return !!(item && item.actions.indexOf(action) >= 0);
  }

  function canAssign(item) { return hasAction(item, 'assign'); }

  function stageName(project, id) {
    for (var i = 0; i < project.stages.length; i++) if (project.stages[i].id === id) return project.stages[i].name;
    return '';
  }

  function unitList() {
    var out = [];
    (function walk(node, depth) {
      if (!node.unit) return;
      if (node.id !== 'org') out.push({ id: node.id, name: node.name, depth: depth });
      (node.children || []).forEach(function (c) { walk(c, node.id === 'org' ? 0 : depth + 1); });
    })(store.view.org, 0);
    return out;
  }

  function unitMembers(unitId) {
    var names = {};
    (function walk(node, inside) {
      var here = inside || node.id === unitId;
      if (!node.unit) { if (here) names[node.name] = true; return; }
      (node.children || []).forEach(function (c) { walk(c, here); });
    })(store.view.org, !unitId);
    return names;
  }

  function unitName(id) {
    var list = unitList();
    for (var i = 0; i < list.length; i++) if (list[i].id === id) return list[i].name;
    return id;
  }

  function personByName(name) {
    var list = store.view ? store.view.people : store.hello.people;
    for (var i = 0; i < list.length; i++) if (list[i].name === name) return list[i];
    return null;
  }

  function byPriority(a, b) {
    return (PRIORITY_RANK[b.priority] || 0) - (PRIORITY_RANK[a.priority] || 0);
  }

  function reducedMotion() {
    return !!(window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches);
  }

  function legalMoves(item) {
    var moves = {};
    if (!item) return moves;
    var col = COLUMN_OF[item.status];
    if (col === 'todo') {
      if (hasAction(item, 'assign')) moves.assigned = 'pickAssignee';
      else if (hasAction(item, 'claim')) { moves.assigned = 'claim'; moves.progress = 'claimStart'; }
    } else if (col === 'assigned') {
      if (hasAction(item, 'begin')) moves.progress = 'begin';
    } else if (col === 'progress') {
      if (hasAction(item, 'done') && !item.openSubtasks.length) moves.review = 'done';
    } else if (col === 'review') {
      if (hasAction(item, 'approve')) moves.done = 'approve';
      if (hasAction(item, 'reject')) moves.assigned = 'reject';
      if (hasAction(item, 'withdraw')) moves.progress = 'withdraw';
    }
    return moves;
  }

  function saveView() {
    var saved = { scroll: {}, focus: null };
    var nodes = document.querySelectorAll('[data-scroll]');
    for (var i = 0; i < nodes.length; i++) {
      saved.scroll[nodes[i].getAttribute('data-scroll')] = { top: nodes[i].scrollTop, left: nodes[i].scrollLeft };
    }
    var active = document.activeElement;
    if (active && active.id && document.getElementById('app').contains(active)) {
      saved.focus = { id: active.id };
      try {
        if (typeof active.selectionStart === 'number') {
          saved.focus.start = active.selectionStart;
          saved.focus.end = active.selectionEnd;
        }
      } catch (e) { }
    }
    return saved;
  }

  function restoreView(saved) {
    var nodes = document.querySelectorAll('[data-scroll]');
    for (var i = 0; i < nodes.length; i++) {
      var s = saved.scroll[nodes[i].getAttribute('data-scroll')];
      if (s) { nodes[i].scrollTop = s.top; nodes[i].scrollLeft = s.left; }
    }
    if (saved.focus) {
      var el = document.getElementById(saved.focus.id);
      if (el) {
        el.focus({ preventScroll: true });
        try { if (saved.focus.start !== undefined) el.setSelectionRange(saved.focus.start, saved.focus.end); } catch (e) { }
      }
    }
  }

  function render() {
    if (drag && drag.started) { store.dirty = true; return; }
    store.dirty = false;
    var app = document.getElementById('app');
    var saved = saveView();
    var hadDialog = !!document.querySelector('.card-dialog');

    var html = '';
    if (!store.connected) html += '<div class="banner" role="status">Connecting to the TIVIDY server...</div>';
    if (!store.hello) {
      html += '<div class="login"><h1>TIVIDY</h1><p>Waiting for the server.</p></div>';
    } else if (!store.view) {
      html += renderLogin();
    } else {
      html += renderShell() + renderDialog();
    }
    app.innerHTML = html;
    renderModal();
    restoreView(saved);
    afterRender(hadDialog);
  }

  function afterRender(hadDialog) {
    var dialog = document.querySelector('.card-dialog');
    placePicker();
    var scroller = document.querySelector('.dlg-scroll');
    if (scroller) scroller.addEventListener('scroll', placePicker);
    if (dialog && !hadDialog) {
      var from = store.openFrom;
      store.openFrom = null;
      if (from && !reducedMotion() && dialog.animate) {
        var to = dialog.getBoundingClientRect();
        var sx = Math.max(0.2, from.width / to.width);
        var sy = Math.max(0.1, from.height / to.height);
        dialog.animate([
          { transformOrigin: '0 0', transform: 'translate(' + (from.left - to.left) + 'px,' + (from.top - to.top) + 'px) scale(' + sx + ',' + sy + ')', opacity: 0.35 },
          { transformOrigin: '0 0', transform: 'none', opacity: 1 }
        ], { duration: 220, easing: 'cubic-bezier(0.2, 0.8, 0.2, 1)' });
        var overlay = document.querySelector('.dialog-overlay');
        if (overlay && overlay.animate) overlay.animate([{ opacity: 0 }, { opacity: 1 }], { duration: 180 });
      }
      if (!store.picker && !store.rejectOpen) dialog.focus({ preventScroll: true });
    }
    if (store.returnTo && !dialog) {
      var back = store.returnTo;
      store.returnTo = null;
      var opener = document.querySelector('[data-act="select"][data-project="' + back.project + '"][data-item="' + back.item + '"]');
      if (opener && !document.querySelector('#modal-root')) opener.focus({ preventScroll: true });
    }
    if (store.revealBoard) {
      store.revealBoard = false;
      var wrap = document.querySelector('.board-wrap');
      var first = null;
      var cols = document.querySelectorAll('.column');
      for (var i = 0; i < cols.length && !first; i++) {
        if (!cols[i].classList.contains('collapsed') && cols[i].querySelector('.card')) first = cols[i];
      }
      if (wrap && first) {
        var wr = wrap.getBoundingClientRect();
        var fr = first.getBoundingClientRect();
        if (fr.right > wr.right || fr.left < wr.left) wrap.scrollLeft += fr.left - wr.left - 16;
      }
    }
    if (store.focusNext) {
      var el = document.getElementById(store.focusNext);
      store.focusNext = null;
      if (el) {
        el.focus({ preventScroll: true });
        if (el.scrollIntoView) el.scrollIntoView({ block: 'nearest' });
      }
    }
  }

  function renderLogin() {
    var people = store.hello.people.slice().sort(function (a, b) {
      var ra = a.owner ? 0 : a.manager ? 1 : 2;
      var rb = b.owner ? 0 : b.manager ? 1 : 2;
      if (ra !== rb) return ra - rb;
      return a.unitPath.localeCompare(b.unitPath) || a.name.localeCompare(b.name);
    });
    var cards = people.map(function (p) {
      var badge = p.owner ? ' <span class="chip amber">Owner</span>' : p.manager ? ' <span class="chip accent">Manager</span>' : '';
      return '<button class="person-card" data-act="login" data-name="' + esc(p.name) + '">' + avatar(p.name, 'lg') +
        '<span class="who"><b>' + esc(p.name) + badge + '</b>' +
        '<span>' + esc(p.role) + (p.unitPath ? ' - ' + esc(p.unitPath) : '') + '</span></span></button>';
    }).join('');
    return '<div class="login"><h1>' + esc(store.hello.company) + '</h1>' +
      '<p>Who are you? Open another port in a second tab to work as someone else at the same time.</p>' +
      '<div class="people-grid">' + cards + '</div></div>';
  }

  function historyLabel(v) {
    var undo = v.history.undo[0];
    var redo = v.history.redo[0];
    var text = v.history.canUndo ? 'Undo: ' + undo : v.history.canRedo ? 'Redo: ' + redo :
      undo ? 'The last action was someone else\'s' : 'Nothing to undo';
    return '<span class="history-label" title="' + esc(text) + '">' + esc(text) + '</span>';
  }

  function pageTitle() {
    if (store.page === 'board') {
      var p = findProject(store.project);
      return p ? p.name : 'Projects';
    }
    return { approvals: 'Approvals', company: 'Company', activity: 'Activity' }[store.page] || 'My work';
  }

  function renderShell() {
    var v = store.view;
    var manager = isManager();
    var nav = function (page, label, count, hot) {
      return '<button class="nav' + (store.page === page ? ' active' : '') + '" data-act="page" data-page="' + page + '">' +
        esc(label) + (count !== undefined ? '<span class="count' + (hot && count ? ' hot' : '') + '">' + count + '</span>' : '') + '</button>';
    };
    var projects = v.projects.map(function (p) {
      var active = store.page === 'board' && store.project === p.id;
      var tag = p.state === 'Completed' ? ' <span class="chip green">Done</span>' :
        p.state === 'Cancelled' ? ' <span class="chip">Cancelled</span>' :
        p.state === 'AwaitingSignOff' ? ' <span class="chip amber">Sign-off</span>' : '';
      return '<button class="nav' + (active ? ' active' : '') + '" data-act="project" data-id="' + esc(p.id) + '">' +
        progressRing(p.progress) + '<span class="nav-text">' + esc(p.name) + '</span>' + tag + '</button>';
    }).join('');

    var undo = v.history.undo[0];
    var redo = v.history.redo[0];
    var history = '<div class="history">' +
      '<button class="ghost" data-act="undo" ' + (v.history.canUndo ? 'title="Undo: ' + esc(undo) + ' (Ctrl+Z)"' : 'disabled title="' +
        (undo ? 'Only a manager can undo someone else\'s action' : 'Nothing to undo') + '"') + '>Undo</button>' +
      '<button class="ghost" data-act="redo" ' + (v.history.canRedo ? 'title="Redo: ' + esc(redo) + ' (Ctrl+Shift+Z)"' : 'disabled') + '>Redo</button>' +
      historyLabel(v) + '</div>';

    var sidebar = '<aside class="sidebar' + (store.navOpen ? ' open' : '') + '" aria-label="Navigation">' +
      '<div class="brand"><span class="logo">T</span><span class="nav-text">' + esc(v.company) + '</span></div>' +
      nav('mywork', 'My work', v.myWork.length) +
      nav('approvals', 'Approvals', v.approvals.length + v.signOffs.length, true) +
      nav('company', 'Company') +
      nav('activity', 'Activity') +
      '<div class="side-label">Projects' + (manager ? '<button class="ghost icon" data-act="modal" data-modal="project" title="New project" aria-label="New project">' + ICON.plus + '</button>' : '') + '</div>' +
      '<div class="side-scroll" data-scroll="side">' + (projects || '<div class="muted side-empty">No projects yet</div>') + '</div>' +
      history +
      '<div class="me">' + avatar(v.me.name) + '<div class="who"><b>' + esc(v.me.name) + '</b><span>' + esc(v.me.role) +
      (v.me.unitPath ? ' - ' + esc(v.me.unitPath) : '') + '</span></div>' +
      '<button class="ghost" data-act="logout" title="Switch person">Switch</button></div>' +
      '</aside>';

    var mobileBar = '<div class="mobile-bar"><button class="ghost icon" data-act="nav-toggle" aria-label="Menu">' + ICON.menu + '</button>' +
      '<b class="nav-text">' + esc(pageTitle()) + '</b><span class="spacer"></span>' + avatar(v.me.name) + '</div>';

    var main;
    if (store.page === 'board') main = renderBoardPage();
    else if (store.page === 'approvals') main = renderApprovalsPage();
    else if (store.page === 'company') main = renderCompanyPage();
    else if (store.page === 'activity') main = renderActivityPage();
    else main = renderMyWorkPage();

    return '<div class="shell">' + mobileBar + sidebar +
      (store.navOpen ? '<div class="nav-scrim" data-act="nav-close"></div>' : '') +
      '<main class="main">' + main + '</main></div>';
  }

  function projectChip(state) {
    var s = PROJECT_STATE[state] || ['', state];
    return '<span class="chip ' + s[0] + '">' + esc(s[1]) + '</span>';
  }

  function renderBoardPage() {
    var p = findProject(store.project);
    if (!p) {
      return '<div class="topbar plain"><h2>Projects</h2></div><div class="content"><div class="empty-state"><b>No projects yet</b>' +
        (isManager() ? 'Start one from a template, the engine shares out the work.<div class="empty-actions"><button class="primary" data-act="modal" data-modal="project">' +
          ICON.plus + 'New project</button></div>' : 'A manager has not started any projects yet.') + '</div></div>';
    }
    var manager = isManager();
    var top = '<div class="topbar"><h2>' + esc(p.name) + '</h2>' + projectChip(p.state) +
      '<span class="chip" title="' + esc(POLICY_HINT[p.policy] || '') + '">' + esc(p.policy) + '</span>' +
      (p.signOffRequired ? '<span class="chip" title="The owner signs this project off when every stage is done">Owner sign-off</span>' : '') +
      '<span class="progress-wrap" title="' + Math.round(p.progress * 100) + '% done"><span class="progress"><span style="width:' + (p.progress * 100) + '%"></span></span>' +
      '<span class="muted">' + Math.round(p.progress * 100) + '%</span></span><span class="spacer"></span>' +
      (p.canSignOff ? '<button class="primary" data-act="signoff">Sign off project</button>' +
        '<button class="danger" data-act="modal" data-modal="flag">Flag an issue</button>' : '') +
      (manager && p.state === 'Running' ? '<button class="primary" data-act="modal" data-modal="task">' + ICON.plus + 'New task</button>' +
        '<button class="ghost danger" data-act="modal" data-modal="cancel">Cancel project</button>' : '') +
      '</div>';

    var notice = '';
    if (p.state === 'AwaitingSignOff') {
      notice = '<div class="notice amber board-notice">All work is done. ' +
        (p.canSignOff ? 'Sign it off, or flag an issue and it goes to the right team\'s manager.' :
          'Waiting for the owner to sign it off or flag issues.') + '</div>';
    }

    var topStages = p.stages.filter(function (s) { return !s.parent; });
    var pills = topStages.map(function (s, i) {
      var cls = s.complete && s.id !== p.currentStage ? 'done' : s.id === p.currentStage ? 'active' : '';
      return (i ? '<span class="sep">' + ICON.chevron + '</span>' : '') + '<span class="stage-pill ' + cls + '" title="' + esc(s.mode) + ' stage">' +
        (cls === 'done' ? statusIcon('Completed') : cls === 'active' ? statusIcon('InProgress') : statusIcon('Created')) +
        esc(s.name) + ' <span class="mode">' + esc(s.mode) + '</span></span>';
    }).join('');

    var buckets = {};
    COLUMNS.forEach(function (c) { buckets[c[0]] = []; });
    p.items.forEach(function (item) { buckets[COLUMN_OF[item.status] || 'waiting'].push(item); });

    var anyDraggable = false;
    var columns = COLUMNS.map(function (c) {
      var items = buckets[c[0]].slice().sort(byPriority);
      var collapsed = !!store.collapsed[c[0]];
      if (collapsed) {
        return '<section class="column collapsed" data-col="' + c[0] + '"><button class="column-toggle" data-act="toggle-col" data-col="' + c[0] +
          '" aria-expanded="false" title="Show ' + esc(c[1]) + '">' + statusIcon(columnStatus(c[0])) +
          '<span class="vlabel">' + esc(c[1]) + '</span><span class="n">' + items.length + '</span></button></section>';
      }
      var cards = items.map(function (item) {
        var html = renderCard(p, item);
        if (html.indexOf('data-drag') >= 0) anyDraggable = true;
        return html;
      }).join('');
      return '<section class="column" data-col="' + c[0] + '"><div class="column-head">' +
        '<button class="column-toggle inline" data-act="toggle-col" data-col="' + c[0] + '" aria-expanded="true" title="Collapse">' +
        statusIcon(columnStatus(c[0])) + esc(c[1]) + '<span class="n">' + items.length + '</span></button>' +
        '<span class="drop-hint"></span></div><div class="cards" data-scroll="col-' + c[0] + '">' +
        (cards || '<div class="empty-col">Nothing here</div>') + '</div></section>';
    }).join('');

    var hint = anyDraggable ? '<span class="board-hint">Drag a card to move it on, or click it to open it</span>' : '';
    return top + notice + '<div class="stages">' + pills + hint + '</div>' +
      '<div class="board-wrap" data-scroll="board"><div class="board">' + columns + '</div></div>';
  }

  function columnStatus(col) {
    return { waiting: 'Created', todo: 'Available', assigned: 'Assigned', progress: 'InProgress', review: 'AwaitingApproval', done: 'Completed' }[col];
  }

  function cardTags(p, item) {
    var tags = priorityChip(item.priority);
    if (item.kind === 'issue') tags += '<span class="chip red">Sign-off issue</span>';
    if (item.status === 'Rejected') tags += '<span class="chip red">Changes requested</span>';
    if (item.status === 'Escalated') tags += '<span class="chip amber">Escalated</span>';
    if (item.openBlockers.length) tags += '<span class="chip amber" title="Blocked by ' + esc(item.openBlockers.join(', ')) + '">Blocked by ' + item.openBlockers.length + '</span>';
    if (item.subtasks.length) {
      tags += '<span class="chip' + (item.openSubtasks.length ? ' amber' : ' green') + '" title="Sub-tasks done">' +
        (item.subtasks.length - item.openSubtasks.length) + '/' + item.subtasks.length + ' sub-tasks</span>';
    }
    if (item.status === 'AwaitingApproval' && item.pendingApprovers.length) {
      tags += '<span class="chip accent" title="Waiting for ' + esc(item.pendingApprovers.join(', ')) + '">' + item.signatures.length + '/' +
        (item.signatures.length + item.pendingApprovers.length) + ' approvals</span>';
    }
    return tags;
  }

  function yourTurn(item) {
    var mine = item.assignee === me().name;
    var personal = item.actions.filter(function (a) {
      return a !== 'assign' && a !== 'autoAssign' && a !== 'priority' && a !== 'subtask' && a !== 'escalate' && !(a === 'claim' && isManager());
    });
    return personal.length > 0 && !mine;
  }

  function renderCard(p, item) {
    var selected = store.selected && store.selected.project === p.id && store.selected.item === item.id;
    var mine = item.assignee === me().name;
    var moves = Object.keys(legalMoves(item));
    var tags = cardTags(p, item);
    if (yourTurn(item)) tags += '<span class="chip green">Your turn</span>';
    var parent = item.parentItem ? findItem(p.id, item.parentItem) : null;
    var where = parent ? 'Sub-task of ' + parent.id : stageName(p, item.stage);
    return '<div class="card' + (selected ? ' selected' : '') + (mine ? ' mine' : '') + (item.status === 'Cancelled' ? ' dim' : '') +
      (moves.length ? ' draggable' : '') + '" role="button" tabindex="0" data-act="select" data-project="' +
      esc(p.id) + '" data-item="' + esc(item.id) + '"' + (moves.length ? ' data-drag="' + moves.join(' ') + '"' : '') +
      ' aria-label="' + esc(item.name + ', ' + (STATUS_LABEL[item.status] || item.status)) + '">' +
      '<div class="row"><span class="mono">' + esc(item.id) + '</span><span class="where">' + esc(where) + '</span></div>' +
      '<div class="row title-row">' + statusIcon(item.status) + '<span class="title">' + esc(item.name) + '</span></div>' +
      '<div class="meta">' + (item.unit ? '<span class="chip">' + esc(unitName(item.unit)) + '</span>' : '') +
      tags + '<span class="who">' + avatar(item.assignee) + '</span></div></div>';
  }

  function prop(k, v, id) {
    return '<div class="k"' + (id ? ' id="' + id + '"' : '') + '>' + esc(k) + '</div><div class="v">' + v + '</div>';
  }

  function renderPicker(p, item) {
    var query = (store.picker.query || '').toLowerCase();
    var inUnit = unitMembers(item.unit);
    var fits = function (person) { return inUnit[person.name] && (!item.role || person.role === item.role); };
    var matches = function (person) {
      return !query || (person.name + ' ' + person.role + ' ' + person.unitPath).toLowerCase().indexOf(query) >= 0;
    };
    var people = store.view.people.filter(matches).sort(function (a, b) {
      return a.load - b.load || a.name.localeCompare(b.name);
    });
    var options = [];
    if (!query || 'auto pipeline'.indexOf(query) >= 0) {
      options.push({ act: 'auto', html: '<span class="pick-icon">' + ICON.spark + '</span><span class="pick-main"><b>Auto-assign</b><span>Let the ' +
        esc(p.policy) + ' pipeline choose' + (item.unit ? ', ' + esc(unitName(item.unit)) + ' first' : '') + '</span></span>' });
    }
    var suggested = people.filter(fits);
    var others = people.filter(function (person) { return !fits(person); });
    var row = function (person) {
      var current = person.name === item.assignee;
      return {
        act: 'pick', name: person.name,
        html: avatar(person.name) + '<span class="pick-main"><b>' + esc(person.name) + (person.name === me().name ? ' (you)' : '') +
          (current ? ' <span class="chip accent">Current</span>' : '') +
          (item.role && person.role !== item.role ? ' <span class="chip amber">Not a ' + esc(item.role) + '</span>' : '') + '</b><span>' + esc(person.role) +
          (person.unitPath ? ' - ' + esc(person.unitPath) : '') + '</span></span><span class="load" title="Open tasks">' + person.load + ' open</span>'
      };
    };
    var groups = [];
    if (options.length) groups.push({ head: '', rows: options });
    if (suggested.length) groups.push({ head: 'Fits ' + (item.unit ? unitName(item.unit) : 'the team') + (item.role ? ', ' + item.role : ''), rows: suggested.map(row) });
    if (others.length) groups.push({ head: suggested.length ? 'Everyone else' : 'People', rows: others.map(row) });

    var index = 0;
    var total = 0;
    groups.forEach(function (g) { total += g.rows.length; });
    var active = Math.max(0, Math.min(store.picker.index || 0, total - 1));
    var list = groups.map(function (g) {
      return (g.head ? '<div class="pick-head">' + esc(g.head) + '</div>' : '') + g.rows.map(function (r) {
        var i = index++;
        return '<button type="button" class="pick' + (i === active ? ' active' : '') + '" role="option" aria-selected="' + (i === active) +
          '" data-act="pick" data-pick="' + r.act + '"' + (r.name ? ' data-name="' + esc(r.name) + '"' : '') + ' data-index="' + i + '">' + r.html + '</button>';
      }).join('');
    }).join('');
    return '<div class="picker' + (narrow() ? '' : ' floating') + '" role="dialog" aria-label="Assign">' +
      '<input id="picker-search" autocomplete="off" placeholder="Assign to..." value="' + esc(store.picker.query || '') + '" aria-label="Search people">' +
      '<div class="pick-list" role="listbox" data-scroll="picker">' + (list || '<div class="pick-empty">Nobody matches</div>') + '</div></div>';
  }

  function topStageOf(p, stageId) {
    var s = null;
    for (var i = 0; i < p.stages.length; i++) if (p.stages[i].id === stageId) s = p.stages[i];
    while (s && s.parent) {
      var parentId = s.parent;
      s = null;
      for (var j = 0; j < p.stages.length; j++) if (p.stages[j].id === parentId) s = p.stages[j];
    }
    return s;
  }

  function waitingReason(p, item) {
    if (p.state !== 'Running') return 'Not open, the project is not running.';
    var own = null;
    for (var i = 0; i < p.stages.length; i++) if (p.stages[i].id === item.stage) own = p.stages[i];
    var top = topStageOf(p, item.stage);
    if (top && !top.active && top.id !== p.currentStage) return 'Not open yet. It opens when the ' + top.name + ' stage starts.';
    if (own && own.mode === 'Sequential') {
      return 'Next in line. ' + own.name + ' runs one item at a time, highest priority first, so this opens when the one before it is done.';
    }
    return 'Not open yet. It opens when the work before it is done.';
  }

  function renderDialog() {
    if (!store.selected) return '';
    var p = findProject(store.selected.project);
    var item = findItem(store.selected.project, store.selected.item);
    if (!p || !item) return '';
    var manager = isManager();
    var parent = item.parentItem ? findItem(p.id, item.parentItem) : null;

    var crumbs = '<span class="crumb">' + esc(p.name) + '</span>' + ICON.chevron +
      (parent ? '<button class="crumb link" data-act="select" data-project="' + esc(p.id) + '" data-item="' + esc(parent.id) + '">' + esc(parent.name) + '</button>' + ICON.chevron :
        '<span class="crumb">' + esc(stageName(p, item.stage)) + '</span>' + ICON.chevron) +
      '<span class="mono">' + esc(item.id) + '</span>';

    var notices = '';
    if (item.status === 'Rejected' && item.reason) notices += '<div class="notice red"><b>Changes requested:</b> ' + esc(item.reason) + '</div>';
    if (item.waiting) notices += '<div class="notice amber">Waiting for ' + esc(item.openBlockers.join(', ')) + ' to be done first.</div>';
    if (item.openSubtasks.length && item.status !== 'Completed' && item.status !== 'Cancelled') {
      notices += '<div class="notice amber">' + item.openSubtasks.length + ' sub-task' + (item.openSubtasks.length > 1 ? 's' : '') +
        ' must be finished before this can go for approval.</div>';
    }
    if (item.status === 'Escalated') notices += '<div class="notice amber">Escalated. A manager needs to give it to someone.</div>';
    if (item.status === 'Created' && !item.waiting) notices += '<div class="notice">' + esc(waitingReason(p, item)) + '</div>';

    var order = ['claim', 'begin', 'done', 'approve', 'withdraw', 'escalate'];
    var buttons = order.filter(function (a) { return hasAction(item, a) && !(a === 'claim' && manager); }).map(function (a) {
      var primary = a === 'approve' || a === 'begin' || a === 'done' || a === 'claim';
      if (a === 'done' && item.openSubtasks.length) {
        return '<button class="primary" disabled title="Finish the sub-tasks first">' + ACTION_LABEL[a] + '</button>';
      }
      return '<button class="' + (primary ? 'primary' : '') + '" data-act="item" data-cmd="' + a + '">' + ACTION_LABEL[a] + '</button>';
    });
    if (canAssign(item) && !item.assignee) {
      buttons.unshift('<button class="primary" data-act="picker-toggle">Assign...</button>');
    }
    if (hasAction(item, 'reject')) {
      buttons.push('<button class="danger' + (store.rejectOpen ? ' pressed' : '') + '" data-act="reject-open" aria-expanded="' + store.rejectOpen + '">Request changes</button>');
    }
    var actionBar = buttons.length ? '<div class="dlg-actions">' + buttons.join('') + '</div>' : '';

    var rejectForm = '';
    if (store.rejectOpen && hasAction(item, 'reject')) {
      rejectForm = '<div class="inline-form reject-form"><label for="reject-reason">What needs to change?</label>' +
        '<textarea id="reject-reason" data-form="reason" rows="3" placeholder="Be specific, the assignee gets this as a notification">' + esc(store.form.reason || '') + '</textarea>' +
        '<div class="row"><button class="danger" data-act="reject">Send back</button><button class="ghost" data-act="reject-cancel">Cancel</button>' +
        '<span class="muted small">Ctrl+Enter sends</span></div></div>';
    }

    var slots = item.signatures.map(function (s) {
      return '<div class="slot done">' + statusIcon('Completed') + '<b>' + esc(s.role) + '</b><span class="muted">signed by ' + esc(s.person) + '</span></div>';
    }).concat(item.pendingApprovers.map(function (r) {
      return '<div class="slot">' + statusIcon('AwaitingApproval') + '<b>' + esc(r) + '</b><span class="muted">waiting</span></div>';
    }));
    if (!slots.length && item.approverRoles.length) {
      slots = item.approverRoles.map(function (r) {
        return '<div class="slot">' + statusIcon('Created') + '<b>' + esc(r) + '</b><span class="muted">when sent for approval</span></div>';
      });
    }
    var approvals = slots.length ? '<section class="section"><h3>Approvals</h3>' + slots.join('') + '</section>' : '';

    var subRows = item.subtasks.map(function (id) {
      var s = findItem(p.id, id);
      if (!s) return '';
      return '<button class="sub-row" data-act="select" data-project="' + esc(p.id) + '" data-item="' + esc(id) + '">' +
        statusIcon(s.status) + '<span class="mono">' + esc(s.id) + '</span><span class="title">' + esc(s.name) + '</span>' +
        priorityChip(s.priority) + avatar(s.assignee) + '</button>';
    }).join('');
    var subForm = '';
    if (hasAction(item, 'subtask')) {
      if (store.subOpen) {
        var people = '';
        if (manager) {
          people = '<select id="sub-assignee" data-form="subAssignee" aria-label="Who does it"><option value="">Let the pipeline choose</option>' +
            store.view.people.map(function (person) {
              return '<option value="' + esc(person.name) + '"' + (store.form.subAssignee === person.name ? ' selected' : '') + '>' +
                esc(person.name) + ' - ' + esc(person.role) + '</option>';
            }).join('') + '</select>';
        }
        subForm = '<div class="inline-form sub-form">' +
          '<input id="sub-name" data-form="subName" placeholder="What still needs doing?" aria-label="Sub-task title" value="' + esc(store.form.subName || '') + '">' +
          '<div class="row"><select id="sub-priority" data-form="subPriority" aria-label="Priority">' + store.view.priorities.map(function (level) {
            var chosen = (store.form.subPriority || item.priority) === level;
            return '<option' + (chosen ? ' selected' : '') + '>' + esc(level) + '</option>';
          }).join('') + '</select>' + people + '</div>' +
          '<div class="row"><button class="primary" data-act="subtask">Add sub-task</button><button class="ghost" data-act="sub-cancel">Cancel</button></div>' +
          '<span class="muted small">' + (item.status === 'AwaitingApproval' ? 'Adding one takes this back out of review. ' : '') +
          'It gets the same team and role as this task' + (manager ? ' unless you pick someone' : '') + '.</span></div>';
      } else {
        subForm = '<button class="ghost add-row" data-act="sub-open">' + ICON.plus + 'Add sub-task</button>';
      }
    }
    var subtasks = (item.subtasks.length || subForm) ? '<section class="section"><h3>Sub-tasks' +
      (item.subtasks.length ? ' <span class="muted">' + (item.subtasks.length - item.openSubtasks.length) + '/' + item.subtasks.length + ' done</span>' : '') +
      '</h3>' + subRows + subForm + '</section>' : '';

    var assigneeValue;
    if (canAssign(item)) {
      assigneeValue = '<button id="assignee-btn" class="prop-btn' + (store.picker ? ' pressed' : '') + '" data-act="picker-toggle" aria-expanded="' + !!store.picker + '">' +
        avatar(item.assignee) + '<span>' + (item.assignee ? esc(item.assignee) : 'Assign...') + '</span></button>';
    } else {
      assigneeValue = item.assignee ? avatar(item.assignee) + esc(item.assignee) : '<span class="muted">Nobody yet</span>';
    }
    if (item.assignee && item.assignedVia) assigneeValue += '<span class="via">via ' + esc(item.assignedVia) + '</span>';
    var floatingPicker = '';
    if (store.picker && canAssign(item)) {
      if (narrow()) assigneeValue += renderPicker(p, item);
      else floatingPicker = renderPicker(p, item);
    }

    var blockers = item.blockedBy.map(function (id) {
      var b = findItem(p.id, id);
      return b ? '<button class="chip link" data-act="select" data-project="' + esc(p.id) + '" data-item="' + esc(id) + '">' + statusIcon(b.status) + esc(b.name) + '</button>' : esc(id);
    }).join(' ');

    var side = '<dl class="props">' +
      prop('Status', statusIcon(item.status) + '<span>' + esc(STATUS_LABEL[item.status] || item.status) + '</span>') +
      prop('Assignee', assigneeValue) +
      prop('Priority', hasAction(item, 'priority') ?
        '<select id="priority-select" data-act-change="priority" aria-label="Priority">' + store.view.priorities.map(function (level) {
          return '<option' + (level === item.priority ? ' selected' : '') + '>' + esc(level) + '</option>';
        }).join('') + '</select>' : priorityChip(item.priority)) +
      prop('Team', item.unit ? esc(item.unitPath || item.unit) : '<span class="muted">Any team</span>') +
      prop('Role', item.role ? esc(item.role) : '<span class="muted">Anyone</span>') +
      prop(parent ? 'Sub-task of' : 'Stage', parent ? '<button class="chip link" data-act="select" data-project="' + esc(p.id) + '" data-item="' + esc(parent.id) + '">' +
        statusIcon(parent.status) + esc(parent.name) + '</button>' : esc(stageName(p, item.stage))) +
      prop('Blocked by', blockers || '<span class="muted">Nothing</span>') +
      (item.dueHours ? prop('Due in', item.dueHours + ' hours') : '') +
      prop('Project', esc(p.name) + ' <span class="chip">' + esc(p.policy) + '</span>') +
      '</dl>';

    var idle = !item.actions.length && p.state === 'Running' && item.status !== 'Completed' && item.status !== 'Cancelled' ?
      '<div class="muted small">Nothing for you to do on this one right now.</div>' : '';

    return '<div class="dialog-overlay" data-act="dialog-overlay">' +
      '<div class="card-dialog" role="dialog" aria-modal="true" aria-labelledby="dlg-title" tabindex="-1">' +
      '<header class="dlg-head"><div class="crumbs">' + crumbs + '</div>' +
      '<button class="ghost icon" data-act="close" aria-label="Close" title="Close (Esc)">' + ICON.close + '</button></header>' +
      '<div class="dlg-scroll" data-scroll="dialog-' + esc(item.id) + '"><div class="dlg-grid">' +
      '<div class="dlg-title"><h2 id="dlg-title">' + statusIcon(item.status) + '<span>' + esc(item.name) + '</span></h2>' +
      '<div class="dlg-tags">' + cardTags(p, item) + '</div>' + notices + actionBar + rejectForm + idle + '</div>' +
      '<div class="dlg-main">' + approvals + subtasks + '</div>' +
      '<aside class="dlg-side">' + side + '</aside>' +
      '</div></div></div>' + floatingPicker + '</div>';
  }

  function narrow() { return window.innerWidth <= 860; }

  function placePicker() {
    var picker = document.querySelector('.picker.floating');
    var anchor = document.getElementById('assignee-btn');
    if (!picker || !anchor) return;
    var a = anchor.getBoundingClientRect();
    var width = picker.offsetWidth;
    var height = picker.offsetHeight;
    var left = Math.max(8, Math.min(a.left - 6, window.innerWidth - width - 8));
    var below = window.innerHeight - a.bottom - 12;
    var top = below >= height || below >= a.top ? a.bottom + 4 : Math.max(8, a.top - height - 4);
    picker.style.left = left + 'px';
    picker.style.top = top + 'px';
    var list = picker.querySelector('.pick-list');
    if (list && top + height > window.innerHeight - 8) list.style.maxHeight = Math.max(120, window.innerHeight - 8 - top - 44) + 'px';
  }

  function refRows(refs, quick, withHeads) {
    var groups = {};
    refs.forEach(function (ref) {
      var item = findItem(ref.project, ref.item);
      if (!item) return;
      var col = COLUMN_OF[item.status] || 'waiting';
      (groups[col] = groups[col] || []).push({ p: findProject(ref.project), item: item });
    });
    return COLUMNS.filter(function (c) { return groups[c[0]]; }).map(function (c) {
      var rows = groups[c[0]].sort(function (a, b) { return byPriority(a.item, b.item); });
      return (withHeads === false ? '' : '<div class="group-head">' + statusIcon(columnStatus(c[0])) + esc(c[1]) + '<span class="muted">' + rows.length + '</span></div>') +
        rows.map(function (row) {
          var actions = row.item.actions.filter(function (a) { return quick.indexOf(a) >= 0 && !(a === 'done' && row.item.openSubtasks.length); }).map(function (a) {
            return '<button class="' + (a === 'withdraw' ? '' : 'primary') + '" data-act="quick" data-cmd="' + a + '" data-project="' +
              esc(row.p.id) + '" data-item="' + esc(row.item.id) + '">' + ACTION_LABEL[a] + '</button>';
          }).join('');
          return '<div class="list-row" role="button" tabindex="0" data-act="select" data-project="' + esc(row.p.id) + '" data-item="' + esc(row.item.id) + '">' +
            statusIcon(row.item.status) + priorityChip(row.item.priority) + '<span class="mono">' + esc(row.item.id) + '</span>' +
            '<span class="title">' + esc(row.item.name) + '</span>' +
            '<span class="row-tags">' +
            (row.item.kind === 'issue' ? '<span class="chip red">Sign-off issue</span>' : '') +
            (row.item.parentItem ? '<span class="chip">Sub-task of ' + esc(row.item.parentItem) + '</span>' : '') +
            (row.item.openSubtasks.length ? '<span class="chip amber">' + row.item.openSubtasks.length + ' open sub-tasks</span>' : '') +
            (row.item.status === 'Rejected' ? '<span class="chip red">Changes requested</span>' : '') +
            (row.item.status === 'AwaitingApproval' ? '<span class="chip accent">' + row.item.signatures.length + '/' +
              (row.item.signatures.length + row.item.pendingApprovers.length) + ' approvals</span>' : '') +
            (row.item.unit ? '<span class="chip">' + esc(unitName(row.item.unit)) + '</span>' : '') +
            '<span class="chip">' + esc(row.p.name) + '</span></span>' + avatar(row.item.assignee) +
            '<span class="actions">' + actions + '</span></div>';
        }).join('');
    }).join('');
  }

  function renderMyWorkPage() {
    var v = store.view;
    var claimable = [];
    v.projects.forEach(function (p) {
      p.items.forEach(function (item) {
        if (item.actions.indexOf('claim') >= 0 && !item.assignee) claimable.push({ project: p.id, item: item.id });
      });
    });
    var top = '<div class="topbar plain"><h2>My work</h2><span class="muted sub">Everything assigned to you that is not finished yet, most urgent first</span></div>';
    var body = v.myWork.length ? '<div class="list">' + refRows(v.myWork, ['begin', 'done', 'withdraw']) + '</div>' :
      '<div class="empty-state"><b>Nothing on your plate</b>Work assigned to you shows up here.</div>';
    if (claimable.length) {
      body += '<div class="group-head spaced">Up for grabs' + (isManager() ? '' : ' in your team') + '<span class="muted">' + claimable.length + '</span></div>' +
        refRows(claimable, isManager() ? [] : ['claim'], false);
    }
    return top + '<div class="content" data-scroll="content-mywork">' + body + '</div>';
  }

  function renderApprovalsPage() {
    var v = store.view;
    var top = '<div class="topbar plain"><h2>Approvals</h2><span class="muted sub">' + (v.me.owner ?
      'Everything waiting for a signature. As the owner you can approve any of it, and you sign finished projects off' :
      'Work routed to your role (' + esc(v.me.role) + ') that has to be signed off before the project can move on') + '</span></div>';
    var signOffs = v.signOffs.map(function (id) {
      var p = findProject(id);
      return '<div class="list-row">' + statusIcon('AwaitingSignOff') + '<span class="title"><b>' + esc(p.name) + '</b> is finished and needs your sign-off</span>' +
        '<span class="chip">' + esc(p.policy) + '</span><span class="actions">' +
        '<button class="primary" data-act="signoff" data-project="' + esc(id) + '">Sign off</button>' +
        '<button class="danger" data-act="modal" data-modal="flag" data-project="' + esc(id) + '">Flag an issue</button></span></div>';
    }).join('');
    var body = '';
    if (signOffs) body += '<div class="group-head">Projects waiting for sign-off<span class="muted">' + v.signOffs.length + '</span></div>' + signOffs;
    if (v.approvals.length) body += '<div class="list">' + refRows(v.approvals, ['approve']) + '</div>';
    if (!body) {
      body = '<div class="empty-state"><b>Nothing waiting for you</b>When work is sent for approval and the policy routes it to a ' +
        esc(v.me.role) + ', you get a notification and it appears here.</div>';
    }
    return top + '<div class="content" data-scroll="content-approvals">' + body + '</div>';
  }

  function renderActivityPage() {
    var v = store.view;
    var history = '<div class="group-head">Undo would revert, newest first' +
      (isManager() ? '' : '<span class="muted">you can undo the top one only when it is yours</span>') + '</div>' +
      (v.history.undo.length ? v.history.undo.map(function (label, i) {
        return '<div class="log-row"><span class="mono">' + (i + 1) + '</span>' + esc(label) + '</div>';
      }).join('') : '<div class="log-row muted">Nothing to undo</div>') +
      (v.history.redo.length ? '<div class="group-head">Redo would reapply</div>' + v.history.redo.map(function (label, i) {
        return '<div class="log-row"><span class="mono">' + (i + 1) + '</span>' + esc(label) + '</div>';
      }).join('') : '');
    var log = v.activity.map(function (line) {
      var at = line.slice(0, 19);
      return '<div class="log-row"><span class="mono">' + esc(at) + '</span>' + esc(line.slice(21)) + '</div>';
    }).join('');
    return '<div class="topbar"><h2>Activity</h2><span class="muted sub">Everything that happened, also saved to the scenario file</span>' +
      '<span class="spacer"></span><button data-act="undo"' + (v.history.canUndo ? '' : ' disabled') + '>Undo</button>' +
        '<button data-act="redo"' + (v.history.canRedo ? '' : ' disabled') + '>Redo</button></div>' +
      '<div class="content" data-scroll="content-activity">' + history + '<div class="group-head">Latest activity, newest first</div>' +
      (log || '<div class="log-row muted">Nothing yet</div>') + '</div>';
  }

  function renderCompanyPage() {
    var manager = isManager();
    var top = '<div class="topbar"><h2>Company</h2><span class="muted sub">Teams are a tree. Pick a team when creating a task and the work is shared out inside it.</span>' +
      '<span class="spacer"></span>' + (manager ? '<button data-act="modal" data-modal="unit" data-parent="">New top-level team</button>' : '') + '</div>';

    function unitBlock(node) {
      var members = (node.children || []).filter(function (c) { return !c.unit; });
      var subs = (node.children || []).filter(function (c) { return c.unit; });
      var chips = members.map(function (m) {
        var person = personByName(m.name);
        var inner = avatar(m.name) + esc(m.name) + ' <small>' + esc(m.role) + (person ? ' - ' + person.load + ' open' : '') + '</small>';
        return manager ? '<button class="member" data-act="modal" data-modal="move" data-name="' + esc(m.name) + '" title="Move to another team">' + inner + '</button>' :
          '<span class="member">' + inner + '</span>';
      }).join('');
      return '<div class="unit"><div class="unit-head"><b>' + esc(node.name) + '</b><span class="mono">' + esc(node.id) + '</span>' +
        '<span class="spacer"></span>' +
        (manager ? '<button class="ghost" data-act="modal" data-modal="unit" data-parent="' + esc(node.id === 'org' ? '' : node.id) + '">+ Team</button>' +
          '<button class="ghost" data-act="modal" data-modal="person" data-unit="' + esc(node.id) + '">+ Person</button>' : '') +
        '</div><div class="unit-body">' + (chips ? '<div class="members">' + chips + '</div>' : '<span class="muted">No direct members</span>') +
        (subs.length ? '<div class="sub-units">' + subs.map(unitBlock).join('') + '</div>' : '') + '</div></div>';
    }

    var root = store.view.org;
    return top + '<div class="content" data-scroll="content-company"><div class="org">' + unitBlock({ id: 'org', name: root.name, children: root.children }) + '</div></div>';
  }

  function renderModal() {
    var root = document.getElementById('modal-root');
    if (!store.modal) {
      if (root) root.remove();
      return;
    }
    if (root && root.getAttribute('data-key') === store.modal.key) return;
    if (!root) {
      root = document.createElement('div');
      root.id = 'modal-root';
      document.body.appendChild(root);
    }
    root.setAttribute('data-key', store.modal.key);
    root.innerHTML = '<div class="overlay" data-act="overlay"><form class="modal" role="dialog" aria-modal="true" aria-label="' + esc(store.modal.title) + '" data-act-submit="' + store.modal.type + '">' +
      '<div class="modal-head">' + esc(store.modal.title) + '</div><div class="modal-body">' + store.modal.body + '</div>' +
      '<div class="modal-foot"><button type="button" data-act="dismiss">Close</button><button class="primary" type="submit">' +
      esc(store.modal.submit) + '</button></div></form></div>';
    var first = root.querySelector('input, select, textarea');
    if (first) first.focus();
  }

  function field(label, control, hint) {
    return '<div class="field"><label>' + esc(label) + '</label>' + control + (hint ? '<span class="hint">' + esc(hint) + '</span>' : '') + '</div>';
  }

  function unitOptions(selected, allowAny) {
    return (allowAny ? '<option value="">Any team</option>' : '') + unitList().map(function (u) {
      return '<option value="' + esc(u.id) + '"' + (u.id === selected ? ' selected' : '') + '>' +
        new Array(u.depth + 1).join('\u00a0\u00a0\u00a0') + esc(u.name) + '</option>';
    }).join('');
  }

  function unitChecks(name) {
    return '<div class="checks">' + unitList().map(function (u) {
      return '<label style="padding-left:' + (6 + u.depth * 14) + 'px"><input type="checkbox" name="' + name + '" value="' + esc(u.id) + '">' + esc(u.name) + '</label>';
    }).join('') + '</div>';
  }

  function roleOptions(placeholder) {
    return '<option value="">' + esc(placeholder) + '</option>' + store.view.roles.map(function (r) {
      return '<option value="' + esc(r) + '">' + esc(r) + '</option>';
    }).join('');
  }

  function priorityOptions(selected) {
    return store.view.priorities.map(function (level) {
      return '<option' + (level === selected ? ' selected' : '') + '>' + esc(level) + '</option>';
    }).join('');
  }

  function openModal(type, data) {
    var v = store.view;
    var m = { type: type, key: type + ':' + Date.now(), data: data || {} };
    if (type === 'project') {
      m.title = 'New project';
      m.submit = 'Start project';
      m.body = field('Name', '<input name="title" maxlength="120" placeholder="e.g. Dark mode" required>') +
        field('Template', '<select name="template">' + v.templates.map(function (t) {
          return '<option' + (t === 'Feature Delivery' ? ' selected' : '') + '>' + esc(t) + '</option>';
        }).join('') + '</select>', 'Built with the Builder pattern') +
        field('Policy', '<select name="policy">' + v.policies.map(function (p) {
          return '<option value="' + esc(p) + '">' + esc(p) + ' - ' + esc(POLICY_HINT[p] || '') + '</option>';
        }).join('') + '</select>', 'The policy is an Abstract Factory: it decides stage order, assignment and approvals') +
        field('Owner sign-off', '<select name="signOff"><option value="template">Same as the template</option>' +
          '<option value="required">Needs the owner\'s sign-off</option><option value="skip">No sign-off, done when the work is done</option></select>',
          v.templateInfo.map(function (t) { return t.name + ': ' + (t.signOff ? 'sign-off' : 'no sign-off'); }).join(', '));
    } else if (type === 'task') {
      var p = findProject(store.project);
      var stages = p.stages.map(function (s) {
        var depth = 0;
        for (var cur = s; cur && cur.parent; depth++) {
          cur = p.stages.filter(function (x) { return x.id === cur.parent; })[0];
        }
        return '<option value="' + esc(s.id) + '"' + (s.id === p.currentStage ? ' selected' : '') + '>' +
          new Array(depth + 1).join('\u00a0\u00a0\u00a0') + esc(s.name) + (s.active ? '' : ' (later)') + '</option>';
      }).join('');
      var blockers = p.items.filter(function (i) { return i.status !== 'Cancelled'; }).map(function (i) {
        return '<label><input type="checkbox" name="blockedBy" value="' + esc(i.id) + '">' + statusIcon(i.status) +
          '<span class="mono">' + esc(i.id) + '</span>' + esc(i.name) + '</label>';
      }).join('');
      var approverChecks = v.roles.map(function (r) {
        return '<label><input type="checkbox" name="approvers" value="' + esc(r) + '">' + esc(r) + '</label>';
      }).join('') + '<label><input type="checkbox" name="approvers" value="Auto">Auto approve</label>';
      m.title = 'New task in ' + p.name;
      m.submit = 'Create task';
      m.body = field('Title', '<input name="name" maxlength="120" placeholder="e.g. Fix the login redirect" required>') +
        '<div class="row2">' + field('Priority', '<select name="priority">' + priorityOptions('Medium') + '</select>') +
        field('Stage', '<select name="stage">' + stages + '</select>') + '</div>' +
        '<div class="row2">' + field('Team', '<select name="unit">' + unitOptions('', true) + '</select>') +
        field('Role', '<select name="role">' + roleOptions('Anyone in the team') + '</select>') + '</div>' +
        field('Needs approval from', '<div class="checks">' + approverChecks + '</div>',
          'Tick more than one to need several sign-offs. None ticked lets the ' + p.policy + ' approval chain decide.') +
        field('Blocked by', '<div class="checks">' + (blockers || '<span class="muted">No other items</span>') + '</div>',
          'This task stays in Waiting until these are done') +
        '<label class="inline-check"><input type="checkbox" name="autoAssign" checked> Assign it through the pipeline (team first, then wider)</label>';
    } else if (type === 'flag') {
      var fp = findProject(m.data.project || store.project);
      m.data.project = fp.id;
      m.title = 'Flag an issue on ' + fp.name;
      m.submit = 'Flag issue';
      m.body = '<div class="notice amber">The project goes back to Running. Each team gets its own issue task, assigned to that team\'s manager (or the nearest one above it). You approve the fixes.</div>' +
        field('What is wrong?', '<input name="name" maxlength="120" placeholder="e.g. Charts are unreadable in dark mode" required>') +
        field('Priority', '<select name="priority">' + priorityOptions('High') + '</select>') +
        field('Teams that need to fix it', unitChecks('units'));
    } else if (type === 'cancel') {
      m.title = 'Cancel ' + findProject(store.project).name;
      m.submit = 'Cancel project';
      m.body = field('Reason', '<input name="reason" maxlength="200" placeholder="Why is this being stopped?">') +
        '<div class="notice red">All unfinished work in this project is cancelled. You can still undo it straight after.</div>';
    } else if (type === 'unit') {
      m.title = 'New team';
      m.submit = 'Add team';
      m.body = field('Name', '<input name="name" maxlength="60" placeholder="e.g. Mobile" required>') +
        field('Id', '<input name="id" maxlength="40" placeholder="e.g. mobile" pattern="[a-z0-9-]+" required>', 'Lower case letters, digits and dashes') +
        field('Inside', '<select name="parent"><option value="">Top level</option>' + unitOptions(m.data.parent, false) + '</select>');
    } else if (type === 'person') {
      m.title = 'Add a person';
      m.submit = 'Add person';
      m.body = field('Name', '<input name="name" maxlength="40" required>') +
        field('Role', '<input name="role" maxlength="40" list="roles" placeholder="e.g. Developer" required><datalist id="roles">' +
          v.roles.map(function (r) { return '<option value="' + esc(r) + '">'; }).join('') + '</datalist>',
          'Manager gives manager rights. Only an owner can add another Owner.') +
        field('Team', '<select name="unit"><option value="org"' + (m.data.unit === 'org' ? ' selected' : '') + '>' + esc(v.company) + ' (top level)</option>' +
          unitOptions(m.data.unit, false) + '</select>');
    } else if (type === 'move') {
      var person = personByName(m.data.name);
      m.title = 'Move ' + m.data.name;
      m.submit = 'Move';
      m.body = field('New team', '<select name="unit"><option value="org">' + esc(v.company) + ' (top level)</option>' +
        unitOptions(person ? person.unit : '', false) + '</select>');
    }
    store.modal = m;
    renderModal();
  }

  function checked(form, name) {
    return Array.prototype.slice.call(form.querySelectorAll('input[name=' + name + ']:checked')).map(function (c) { return c.value; });
  }

  function submitModal(form) {
    var m = store.modal;
    var val = function (name) { var el = form.elements[name]; return el ? el.value.trim() : ''; };
    var request;
    if (m.type === 'project') {
      request = run('startProject', { template: val('template'), title: val('title'), policy: val('policy'), signOff: val('signOff') }).then(function (r) {
        if (r.ok) { store.project = r.data.project; store.page = 'board'; write('project', store.project); write('page', 'board'); }
        return r;
      });
    } else if (m.type === 'task') {
      request = run('addTask', {
        project: store.project, name: val('name'), unit: val('unit'), role: val('role'), stage: val('stage'),
        priority: val('priority'), approvers: checked(form, 'approvers'), blockedBy: checked(form, 'blockedBy'),
        autoAssign: form.elements.autoAssign.checked
      }).then(function (r) {
        if (r.ok) {
          var made = findItem(store.project, r.data.item);
          var col = made ? COLUMN_OF[made.status] : null;
          if (col && store.collapsed[col]) { store.collapsed[col] = false; write('collapsed', JSON.stringify(store.collapsed)); }
          openDialog(store.project, r.data.item, null);
        }
        return r;
      });
    } else if (m.type === 'flag') {
      var units = checked(form, 'units');
      if (!units.length) { toast('Pick at least one team', 'error'); return; }
      request = run('flagIssue', { project: m.data.project, name: val('name'), priority: val('priority'), units: units }).then(function (r) {
        if (r.ok) { store.project = m.data.project; store.page = 'board'; write('page', 'board'); write('project', store.project); }
        return r;
      });
    } else if (m.type === 'cancel') {
      request = run('cancel', { project: store.project, reason: val('reason') });
    } else if (m.type === 'unit') {
      request = run('addUnit', { parent: val('parent'), id: val('id'), name: val('name') });
    } else if (m.type === 'person') {
      request = run('addPerson', { name: val('name'), role: val('role'), unit: val('unit') });
    } else if (m.type === 'move') {
      request = run('moveMember', { unit: val('unit'), name: m.data.name });
    }
    request.then(function (r) {
      if (r.ok) {
        store.modal = null;
        render();
      }
    });
  }

  function toast(text, kind, undo) {
    var host = document.getElementById('toasts');
    var el = document.createElement('div');
    el.className = 'toast' + (kind ? ' ' + kind : '');
    el.setAttribute('role', kind === 'error' ? 'alert' : 'status');
    el.innerHTML = '<span class="dot"></span><span class="msg">' + esc(text) + '</span>' +
      (undo ? '<button class="ghost toast-undo" data-act="undo">Undo</button>' : '');
    host.appendChild(el);
    while (host.children.length > 3) host.removeChild(host.firstChild);
    setTimeout(function () { if (el.parentNode) el.parentNode.removeChild(el); }, kind === 'error' ? 6000 : kind === 'mine' ? 6000 : 3000);
  }

  function openDialog(project, item, fromEl, options) {
    var opts = options || {};
    var wasOpen = !!store.selected;
    store.selected = { project: project, item: item };
    store.form = {};
    store.picker = opts.picker ? { query: '', index: 0 } : null;
    store.rejectOpen = !!opts.reject;
    store.subOpen = false;
    store.openFrom = !wasOpen && fromEl ? fromEl.getBoundingClientRect() : null;
    if (opts.picker) store.focusNext = 'picker-search';
    else if (opts.reject) store.focusNext = 'reject-reason';
    render();
  }

  function closeDialog(returnFocus) {
    if (store.selected && returnFocus) store.returnTo = store.selected;
    store.selected = null;
    store.picker = null;
    store.rejectOpen = false;
    store.subOpen = false;
    store.form = {};
  }

  function itemArgs(extra) {
    var args = { project: store.selected.project, item: store.selected.item };
    for (var k in extra) if (Object.prototype.hasOwnProperty.call(extra, k)) args[k] = extra[k];
    return args;
  }

  function pickOption(el) {
    var kind = el.getAttribute('data-pick');
    var request = kind === 'auto' ? run('autoAssign', itemArgs({})) : run('assign', itemArgs({ to: el.getAttribute('data-name') }));
    store.picker = null;
    render();
    request.then(function (r) {
      if (r.ok) {
        var dialog = document.querySelector('.card-dialog');
        if (dialog) dialog.focus({ preventScroll: true });
      }
    });
  }

  function sendReject() {
    var reason = (store.form.reason || '').trim();
    if (!reason) {
      toast('Say what needs to change first', 'error');
      store.focusNext = 'reject-reason';
      render();
      return;
    }
    run('reject', itemArgs({ reason: reason })).then(function (r) {
      if (r.ok) { store.form.reason = ''; store.rejectOpen = false; render(); }
    });
  }

  function performMove(projectId, itemId, kind, cardEl) {
    var args = { project: projectId, item: itemId };
    if (kind === 'pickAssignee') { openDialog(projectId, itemId, cardEl, { picker: true }); return; }
    if (kind === 'reject') { openDialog(projectId, itemId, cardEl, { reject: true }); return; }
    if (kind === 'claimStart') {
      run('claim', args).then(function (r) { if (r.ok) run('begin', args); });
      return;
    }
    run(kind, args);
  }

  function columnAt(x, y) {
    var el = document.elementFromPoint(x, y);
    var col = el && el.closest ? el.closest('.column') : null;
    if (!col) {
      var cols = document.querySelectorAll('.column');
      for (var i = 0; i < cols.length; i++) {
        var r = cols[i].getBoundingClientRect();
        if (x >= r.left && x <= r.right && y >= r.top - 40 && y <= window.innerHeight) return cols[i];
      }
    }
    return col;
  }

  function beginDrag() {
    var d = drag;
    var card = d.card;
    var item = findItem(d.project, d.item);
    d.moves = legalMoves(item);
    if (!Object.keys(d.moves).length) { drag = null; return; }
    d.started = true;
    var rect = card.getBoundingClientRect();
    d.offX = d.x - rect.left;
    d.offY = d.y - rect.top;
    d.from = rect;
    var ghost = card.cloneNode(true);
    ghost.classList.add('ghost');
    ghost.classList.remove('selected');
    ghost.removeAttribute('data-act');
    ghost.removeAttribute('tabindex');
    ghost.setAttribute('aria-hidden', 'true');
    var width = d.touch ? Math.min(rect.width, 200) : rect.width;
    if (width !== rect.width) {
      d.offX = d.offX * width / rect.width;
      d.offY = Math.min(d.offY, 40);
    }
    ghost.style.width = width + 'px';
    ghost.style.left = '0px';
    ghost.style.top = '0px';
    document.body.appendChild(ghost);
    d.ghost = ghost;
    card.classList.add('lifted');
    document.body.classList.add('dragging');
    var cols = document.querySelectorAll('.column');
    for (var i = 0; i < cols.length; i++) {
      var key = cols[i].getAttribute('data-col');
      if (d.moves[key]) {
        cols[i].classList.add('drop-ok');
        var hint = cols[i].querySelector('.drop-hint');
        if (hint) hint.textContent = MOVE_LABEL[d.moves[key]];
        if (cols[i].classList.contains('collapsed')) cols[i].classList.add('peek');
      } else if (cols[i].contains(card)) {
        cols[i].classList.add('drop-origin');
      } else {
        cols[i].classList.add('drop-no');
      }
    }
    if (d.touch && navigator.vibrate) { try { navigator.vibrate(12); } catch (e) { } }
    moveGhost();
    autoScrollLoop();
  }

  function moveGhost() {
    var d = drag;
    if (!d || !d.ghost) return;
    d.ghost.style.transform = 'translate(' + (d.x - d.offX) + 'px,' + (d.y - d.offY) + 'px) rotate(2.5deg)';
    var col = columnAt(d.x, d.y);
    var key = col ? col.getAttribute('data-col') : null;
    var target = key && d.moves[key] ? key : null;
    if (target !== d.target) {
      var old = document.querySelector('.column.drop-hover');
      if (old) old.classList.remove('drop-hover');
      if (target) col.classList.add('drop-hover');
      d.target = target;
    }
  }

  function autoScrollLoop() {
    var d = drag;
    if (!d || !d.started) return;
    var board = document.querySelector('.board-wrap');
    if (board) {
      var r = board.getBoundingClientRect();
      var edge = Math.min(72, r.width / 5);
      var speed = 0;
      if (d.x < r.left + edge) speed = -Math.round(6 + 16 * Math.min(1, (r.left + edge - d.x) / edge));
      else if (d.x > r.right - edge) speed = Math.round(6 + 16 * Math.min(1, (d.x - (r.right - edge)) / edge));
      if (speed) { board.scrollLeft += speed; moveGhost(); }
    }
    var col = columnAt(d.x, d.y);
    var cards = col ? col.querySelector('.cards') : null;
    if (cards) {
      var cr = cards.getBoundingClientRect();
      if (d.y < cr.top + 40) cards.scrollTop -= 8;
      else if (d.y > cr.bottom - 40) cards.scrollTop += 8;
    }
    d.raf = window.requestAnimationFrame(autoScrollLoop);
  }

  function endDrag(commit) {
    var d = drag;
    drag = null;
    if (!d) return;
    if (d.timer) clearTimeout(d.timer);
    if (!d.started) return;
    if (d.raf) window.cancelAnimationFrame(d.raf);
    suppressClick = true;
    setTimeout(function () { suppressClick = false; }, 0);
    document.body.classList.remove('dragging');
    var cols = document.querySelectorAll('.column');
    for (var i = 0; i < cols.length; i++) cols[i].classList.remove('drop-ok', 'drop-no', 'drop-hover', 'drop-origin', 'peek');
    var target = commit ? d.target : null;
    var kind = target ? d.moves[target] : null;
    var ghost = d.ghost;
    var finish = function () {
      if (ghost.parentNode) ghost.parentNode.removeChild(ghost);
      d.card.classList.remove('lifted');
    };
    var immediate = kind && kind !== 'pickAssignee' && kind !== 'reject';
    var land = d.from;
    if (immediate) {
      var dest = document.querySelector('.column[data-col="' + target + '"] .cards');
      if (dest) {
        var empty = dest.querySelector('.empty-col');
        if (empty) empty.parentNode.removeChild(empty);
        dest.insertBefore(d.card, dest.firstChild);
        land = d.card.getBoundingClientRect();
      }
    }
    if (ghost.animate && !reducedMotion()) {
      var anim = ghost.animate([
        { transform: ghost.style.transform },
        { transform: 'translate(' + land.left + 'px,' + land.top + 'px) rotate(0deg)' }
      ], { duration: 160, easing: 'ease-out' });
      anim.onfinish = finish;
    } else {
      finish();
    }
    if (kind) performMove(d.project, d.item, kind, d.card);
    if (store.dirty && !kind) render();
    else if (store.dirty && immediate) setTimeout(render, 170);
  }

  document.addEventListener('pointerdown', function (e) {
    if (e.button !== 0 || drag) return;
    var card = e.target.closest ? e.target.closest('.card[data-drag]') : null;
    if (!card || store.selected || store.modal) return;
    drag = {
      card: card, pointerId: e.pointerId, project: card.getAttribute('data-project'), item: card.getAttribute('data-item'),
      startX: e.clientX, startY: e.clientY, x: e.clientX, y: e.clientY, touch: e.pointerType !== 'mouse', started: false
    };
    if (drag.touch) {
      drag.timer = setTimeout(function () { if (drag && !drag.started) beginDrag(); }, 260);
    }
  });

  document.addEventListener('pointermove', function (e) {
    if (!drag || e.pointerId !== drag.pointerId) return;
    drag.x = e.clientX;
    drag.y = e.clientY;
    var moved = Math.abs(drag.x - drag.startX) + Math.abs(drag.y - drag.startY);
    if (!drag.started) {
      if (drag.touch) {
        if (moved > 10) { clearTimeout(drag.timer); drag = null; }
      } else if (moved > 6) {
        beginDrag();
      }
      return;
    }
    e.preventDefault();
    moveGhost();
  });

  document.addEventListener('pointerup', function (e) {
    if (!drag || e.pointerId !== drag.pointerId) return;
    drag.x = e.clientX;
    drag.y = e.clientY;
    if (drag.started) moveGhost();
    endDrag(true);
  });

  document.addEventListener('pointercancel', function (e) {
    if (!drag || e.pointerId !== drag.pointerId) return;
    if (drag.started && drag.touch) return;
    endDrag(false);
  });

  document.addEventListener('touchmove', function (e) {
    if (!drag || !drag.started) return;
    e.preventDefault();
    var t = e.touches[0];
    if (t) { drag.x = t.clientX; drag.y = t.clientY; moveGhost(); }
  }, { passive: false });

  document.addEventListener('touchend', function (e) {
    if (!drag || !drag.started || !drag.touch) return;
    var t = e.changedTouches[0];
    if (t) { drag.x = t.clientX; drag.y = t.clientY; moveGhost(); }
    endDrag(true);
  });

  document.addEventListener('touchcancel', function () {
    if (drag && drag.started && drag.touch) endDrag(false);
  });

  document.addEventListener('contextmenu', function (e) {
    if (drag) e.preventDefault();
  });

  document.addEventListener('click', function (e) {
    if (suppressClick) { e.preventDefault(); e.stopPropagation(); return; }
    var el = e.target.closest('[data-act]');
    if (store.picker && !e.target.closest('.picker') && !(el && el.getAttribute('data-act') === 'picker-toggle')) {
      store.picker = null;
      render();
      if (el && el.getAttribute('data-act') === 'dialog-overlay') return;
    }
    if (!el) return;
    var act = el.getAttribute('data-act');

    if (act === 'overlay') {
      if (e.target === el) { store.modal = null; render(); }
      return;
    }
    if (act === 'dialog-overlay') {
      if (e.target === el) { closeDialog(true); render(); }
      return;
    }
    if (act === 'dismiss') { store.modal = null; render(); return; }
    if (act === 'login') {
      var name = el.getAttribute('data-name');
      run('login', { as: name }).then(function (r) { if (r.ok) write('person', name); });
      return;
    }
    if (act === 'logout') {
      write('person', null);
      closeDialog();
      store.navOpen = false;
      run('logout', {});
      return;
    }
    if (act === 'nav-toggle') { store.navOpen = !store.navOpen; render(); return; }
    if (act === 'nav-close') { store.navOpen = false; render(); return; }
    if (act === 'page') {
      store.page = el.getAttribute('data-page');
      closeDialog();
      store.navOpen = false;
      write('page', store.page);
      render();
      return;
    }
    if (act === 'project') {
      store.page = 'board';
      store.project = el.getAttribute('data-id');
      store.revealBoard = true;
      closeDialog();
      store.navOpen = false;
      write('page', 'board');
      write('project', store.project);
      render();
      return;
    }
    if (act === 'toggle-col') {
      var col = el.getAttribute('data-col');
      store.collapsed[col] = !store.collapsed[col];
      write('collapsed', JSON.stringify(store.collapsed));
      render();
      return;
    }
    if (act === 'select') {
      if (e.target.closest('[data-act="quick"]')) return;
      e.stopPropagation();
      openDialog(el.getAttribute('data-project'), el.getAttribute('data-item'), el);
      return;
    }
    if (act === 'close') { closeDialog(true); render(); return; }
    if (act === 'quick') {
      e.stopPropagation();
      run(el.getAttribute('data-cmd'), { project: el.getAttribute('data-project'), item: el.getAttribute('data-item') });
      return;
    }
    if (act === 'item') {
      run(el.getAttribute('data-cmd'), itemArgs({}));
      return;
    }
    if (act === 'picker-toggle') {
      store.picker = store.picker ? null : { query: '', index: 0 };
      if (store.picker) store.focusNext = 'picker-search';
      render();
      return;
    }
    if (act === 'pick') { pickOption(el); return; }
    if (act === 'reject-open') {
      store.rejectOpen = !store.rejectOpen;
      if (store.rejectOpen) store.focusNext = 'reject-reason';
      render();
      return;
    }
    if (act === 'reject-cancel') { store.rejectOpen = false; store.form.reason = ''; render(); return; }
    if (act === 'reject') { sendReject(); return; }
    if (act === 'sub-open') { store.subOpen = true; store.focusNext = 'sub-name'; render(); return; }
    if (act === 'sub-cancel') { store.subOpen = false; store.form.subName = ''; render(); return; }
    if (act === 'subtask') {
      var title = (store.form.subName || '').trim();
      if (!title) { toast('Give the sub-task a title first', 'error'); store.focusNext = 'sub-name'; render(); return; }
      var parent = selectedItem();
      run('addSubtask', itemArgs({
        name: title, priority: store.form.subPriority || (parent ? parent.priority : ''), assignee: store.form.subAssignee || ''
      })).then(function (r) {
        if (r.ok) { store.form.subName = ''; store.form.subAssignee = ''; store.focusNext = 'sub-name'; render(); }
      });
      return;
    }
    if (act === 'signoff') {
      run('signOff', { project: el.getAttribute('data-project') || store.project });
      return;
    }
    if (act === 'undo') { run('undo', {}); return; }
    if (act === 'redo') { run('redo', {}); return; }
    if (act === 'modal') {
      openModal(el.getAttribute('data-modal'), {
        parent: el.getAttribute('data-parent') || '',
        unit: el.getAttribute('data-unit') || '',
        name: el.getAttribute('data-name') || '',
        project: el.getAttribute('data-project') || ''
      });
    }
  }, true);

  document.addEventListener('input', function (e) {
    if (e.target.id === 'picker-search' && store.picker) {
      store.picker.query = e.target.value;
      store.picker.index = 0;
      render();
      return;
    }
    var key = e.target.getAttribute && e.target.getAttribute('data-form');
    if (key) store.form[key] = e.target.value;
  });

  document.addEventListener('change', function (e) {
    var key = e.target.getAttribute && e.target.getAttribute('data-form');
    if (key) store.form[key] = e.target.value;
    if (e.target.getAttribute && e.target.getAttribute('data-act-change') === 'priority' && store.selected) {
      run('setPriority', itemArgs({ priority: e.target.value }));
    }
  });

  document.addEventListener('submit', function (e) {
    var form = e.target.closest('[data-act-submit]');
    if (!form) return;
    e.preventDefault();
    submitModal(form);
  });

  function trapFocus(e) {
    var container = store.modal ? document.querySelector('#modal-root .modal') : document.querySelector('.card-dialog');
    if (!container) return;
    var focusable = container.querySelectorAll('button:not([disabled]), input, select, textarea, [tabindex="0"]');
    if (!focusable.length) return;
    var first = focusable[0];
    var last = focusable[focusable.length - 1];
    if (e.shiftKey && (document.activeElement === first || document.activeElement === container)) { e.preventDefault(); last.focus(); }
    else if (!e.shiftKey && document.activeElement === last) { e.preventDefault(); first.focus(); }
  }

  document.addEventListener('keydown', function (e) {
    if (e.key === 'Escape') {
      if (drag) { endDrag(false); return; }
      if (store.modal) { store.modal = null; render(); return; }
      if (store.picker) { store.picker = null; render(); var b = document.getElementById('assignee-btn'); if (b) b.focus(); return; }
      if (store.rejectOpen) { store.rejectOpen = false; render(); return; }
      if (store.subOpen) { store.subOpen = false; render(); return; }
      if (store.selected) { closeDialog(true); render(); return; }
      if (store.navOpen) { store.navOpen = false; render(); return; }
      return;
    }
    if (e.key === 'Tab' && (store.modal || store.selected)) { trapFocus(e); return; }
    if (e.target.id === 'picker-search' && store.picker) {
      var options = document.querySelectorAll('.picker .pick');
      if (e.key === 'ArrowDown' || e.key === 'ArrowUp') {
        e.preventDefault();
        var n = options.length;
        if (!n) return;
        store.picker.index = ((store.picker.index || 0) + (e.key === 'ArrowDown' ? 1 : -1) + n) % n;
        render();
        var active = document.querySelector('.picker .pick.active');
        if (active && active.scrollIntoView) active.scrollIntoView({ block: 'nearest' });
        return;
      }
      if (e.key === 'Enter') {
        e.preventDefault();
        var chosen = document.querySelector('.picker .pick.active');
        if (chosen) pickOption(chosen);
        return;
      }
    }
    if (e.target.id === 'reject-reason' && e.key === 'Enter' && (e.ctrlKey || e.metaKey)) { e.preventDefault(); sendReject(); return; }
    if (e.target.id === 'sub-name' && e.key === 'Enter') {
      e.preventDefault();
      var btn = document.querySelector('[data-act="subtask"]');
      if (btn) btn.click();
      return;
    }
    if ((e.key === 'Enter' || e.key === ' ') && e.target.getAttribute && e.target.getAttribute('role') === 'button' &&
        e.target.getAttribute('data-act') === 'select') {
      e.preventDefault();
      openDialog(e.target.getAttribute('data-project'), e.target.getAttribute('data-item'), e.target);
      return;
    }
    var typing = /INPUT|SELECT|TEXTAREA/.test(e.target.tagName);
    if (!typing && store.view && !store.modal && (e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'z') {
      e.preventDefault();
      run(e.shiftKey ? 'redo' : 'undo', {});
    }
  });

  window.addEventListener('resize', function () {
    if (store.picker) render();
  });

  render();
  connect();
})();
