// canvas & board globals
var c, ctx;
var canvasN = 512;   // total pixel size of canvas
var gpx_size;        // pixel size of one cell
var gboard = [];

// a simple HSL palette for Q states
var stateColors = [];
function initStateColors(Q) {
  stateColors = [];
  for (var i = 0; i < Q; i++) {
    var hue = Math.round(360 * i / Q);
    stateColors.push('hsl(' + hue + ',80%,50%)');
  }
}

// draw one “pixel” cell at (x,y) of size px
function put_pixel(x, y, size, color) {
  ctx.fillStyle = color;
  ctx.fillRect(x * size, y * size, size, size);
}

// render the entire gboard[]. Assumes gboard.length == gN*gN
function display_board(N, board) {
  for (var y = 0; y < N; y++) {
    for (var x = 0; x < N; x++) {
      var state = board[x + y * N];
      put_pixel(x, y, gpx_size, stateColors[state]);
    }
  }
}

// clear canvas background
function clear() {
  ctx.fillStyle = '#eee';
  ctx.fillRect(0, 0, canvasN, canvasN);
}

// initialize the HTML canvas & context
function init_canvas() {
  c = document.getElementById('canvas');
  c.width = c.height = canvasN;
  ctx = c.getContext('2d');
  clear();
}

// initialize a random or provided board and draw it
function init_board(N, Q, boardData) {
  gN = N;
  gQ = Q;
  gpx_size = canvasN / gN;
  init_canvas();
  initStateColors(Q);

  if (boardData && boardData.length === N * N) {
    gboard = boardData.slice();
  } 
  display_board(gN, gboard);
}

// call this after you update gboard[]
function draw_all(gboard) {
  clear();
  display_board(gN, gboard);
}