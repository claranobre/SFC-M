class TextBox {
 
  // demands rectMode(CORNER)
 
  final color textC, baseC, bordC, slctC;
  final short x, y, w, h, xw, yh, lim;
 
  boolean isFocused;
  String txt = "";
  String title; 
 
  TextBox(
    String tt, 
    int xx, int yy, 
    int ww, int hh, 
    int li, 
    color te, color ba, color bo, color se) {
 
    title=tt;
 
    x = (short) xx;
    y = (short) yy;
    w = (short) ww;
    h = (short) hh;
 
    lim = (short) li;
 
    xw = (short) (xx + ww);
    yh = (short) (yy + hh);
 
    textC = te;
    baseC = ba;
    bordC = bo;
    slctC = se;
  }
 
  void display() {
    stroke(isFocused? slctC : bordC);
 
    // outer 
    fill(baseC);
    rect(x-10, y-90, w+20, h+100);
 
    fill(0); 
    text(title, x, y-90+20);
 
    // main / inner
    fill(baseC);
    rect(x, y, w, h);
 
 
    fill(textC);
    text(txt + blinkChar(), x, y, w, h);
  }
 
  /*void tKeyTyped() {
 
    char k = key;
 
    if (k == ESC) {
      // println("esc 2");
      state=stateNormal; 
      key=0;
      return;
    } 
 
    if (k == CODED)  return;
 
    final int len = txt.length();
 
    if (k == BACKSPACE)  txt = txt.substring(0, max(0, len-1));
    else if (len >= lim)  return;
    else if (k == ENTER || k == RETURN) {
      // this ends the entering 
      println("RET ");
      state  = stateNormal; // close input box 
      result = txt;
    } else if (k == TAB & len < lim-3)  txt += "    ";
    else if (k == DELETE)  txt = "";
    else if (k >= ' ')     txt += str(k);
  }
 
 
  void tKeyPressed() {
    if (key == ESC) {
      println("esc 3");
      state=stateNormal;
      key=0;
    }
 
    if (key != CODED)  return;
 
    final int k = keyCode;
 
    final int len = txt.length();
 
    if (k == LEFT)  txt = txt.substring(0, max(0, len-1));
    else if (k == RIGHT & len < lim-3)  txt += "    ";
  }*/
 
  String blinkChar() {
    return isFocused && (frameCount>>2 & 1) == 0 ? "_" : "";
  }
 
  boolean checkFocus() {
    return isFocused = mouseX > x & mouseX < xw & mouseY > y & mouseY < yh;
  }
}