// The clipping call to the system library doesn't seem to do anything so until I work out why,
// I'll duplicate the code locally in cineops.c and do our own clipping.

// Note that we may want to draw outside the game's window to annotate with a key legend
// or menu options for keys that are not supported by our controller, so it isn't such a problem
// that the clipping here will not apply to every single vector.

enum { TOP = 0x1, BOTTOM = 0x2, RIGHT = 0x4, LEFT = 0x8 };
//static enum { FALSE, TRUE };
typedef unsigned int outcode;

static outcode compute_outcode (int x, int y, int xmin, int ymin, int xmax, int ymax) {
  outcode oc = 0;

  if (y > ymax)
    oc |= TOP;
  else if (y < ymin)
    oc |= BOTTOM;
  if (x > xmax)
    oc |= RIGHT;
  else if (x < xmin)
    oc |= LEFT;
  return oc;
}

static int retain_after_clipping (int *x1p, int *y1p, int *x2p, int *y2p, int xmin, int ymin, int xmax, int ymax) {
#define x1 (*x1p)
#define y1 (*y1p)
#define x2 (*x2p)
#define y2 (*y2p)

  int accept;
  int done;
  outcode outcode1, outcode2;

  //
  // Is this algorithm faulty in the case of:
  //
  //         |            |
  //         |            |
  //         |            |
  // --------+------------+--------
  //         |            |
  //         |            |
  //         |            |   * x1,y1
  // --------+------------+--/-----
  //         |            | /
  //         |            |/
  //         |            /
  //         |           /|
  //         |    x2,y2 * |
  //
  
  // because the codes are the same as this case, which does need clipping:

  //         |            |
  //         |            |
  //         |            |
  // --------+------------+--------
  //         |            |   * x1,y1
  //         |            |  /
  //         |            | /
  //         |            |/
  //         |            /
  //         |           /|
  // --------+----------/-+--------
  //         |         /  |
  //         |  x2,y2 *   |
  //

  // Either the first case above is clipped unnecessarily or the second case is not clipped when it should be.

  accept = FALSE;
  done = FALSE;
  outcode1 = compute_outcode (x1, y1, xmin, ymin, xmax, ymax);
  outcode2 = compute_outcode (x2, y2, xmin, ymin, xmax, ymax);
  do {
    if (outcode1 == 0 && outcode2 == 0) {
      accept = TRUE;
      done = TRUE;
    } else if (outcode1 & outcode2) {
      done = TRUE;
    } else {
      int x, y;
      int outcode_ex = outcode1 ? outcode1 : outcode2;

      if (outcode_ex & TOP) {
        x = x1 + (x2 - x1) * (ymax - y1) / (y2 - y1);
        y = ymax;
      } else if (outcode_ex & BOTTOM) {
        x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1);
        y = ymin;
      } else if (outcode_ex & RIGHT) {
        y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1);
        x = xmax;
      } else {
        y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1);
        x = xmin;
      }
      if (outcode_ex == outcode1) {
        x1 = x;
        y1 = y;
        outcode1 = compute_outcode (x1, y1, xmin, ymin, xmax, ymax);
      } else {
        x2 = x;
        y2 = y;
        outcode2 = compute_outcode (x2, y2, xmin, ymin, xmax, ymax);
      }
    }
  } while (done == FALSE);
  return accept && done;
#undef x1
#undef y1
#undef x2
#undef y2
}


#ifdef NEVER
// https://stackoverflow.com/questions/47884592/how-to-reverse-cohen-sutherland-algorithm
// adds new baselines
// invalidates old baseline

// To do this properly, this call has to draw the vector, not just mark it as needing clip.
// Needs a callback parameter ADD_CLIPPED_VECTOR
static void reverse_cohen_sutherland (VectorPipelineBase * baseVector, int xmin, int ymin, int xmax, int ymax) {
  // used to blank a window inside the image, for drawing a pop-up over.
#define x1 (*x1p)
#define y1 (*y1p)
#define x2 (*x2p)
#define y2 (*y2p)

  baseVector->force |= PL_BASE_FORCE_EMPTY;

  int accept;
  int done;
  outcode outcode1, outcode2;

  accept = FALSE;
  done = FALSE;
  outcode1 = compute_outcode (x1, y1, xmin, ymin, xmax, ymax);
  outcode2 = compute_outcode (x2, y2, xmin, ymin, xmax, ymax);
  do {
    if (outcode1 == 0 && outcode2 == 0) {
      done = TRUE;
    } else if (outcode1 & outcode2) {
      accept = TRUE;
      done = TRUE;
    } else {
      int x, y;
      int outcode_ex = outcode1 ? outcode1 : outcode2;

      if (outcode_ex & TOP) {
        x = x1 + (x2 - x1) * (ymax - y1) / (y2 - y1);
        y = ymax;
      } else if (outcode_ex & BOTTOM) {
        x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1);
        y = ymin;
      } else if (outcode_ex & RIGHT) {
        y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1);
        x = xmax;
      } else {
        y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1);
        x = xmin;
      }

      if (outcode_ex == outcode1) {
	// Need a callback to draw the two parts of a vector that crosses the whole window
        ADD_CLIPPED_VECTOR (x1, y1, x, y, baseVector);
        x1 = x;
        y1 = y;
        outcode1 = compute_outcode (x1, y1, xmin, ymin, xmax, ymax);
      } else {
        ADD_CLIPPED_VECTOR (x, y, x2, y2, baseVector);
        x2 = x;
        y2 = y;
        outcode2 = compute_outcode (x2, y2, xmin, ymin, xmax, ymax);
      }
    }
  } while (done == FALSE);
  if (accept == TRUE) {
    ADD_CLIPPED_VECTOR (x1, y1, x2, y2, baseVector);
  }
  return;
#undef x1
#undef y1
#undef x2
#undef y2
}
#endif
