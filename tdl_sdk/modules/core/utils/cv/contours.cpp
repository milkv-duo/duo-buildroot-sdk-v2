#include "contours.hpp"
#include "shapedescr.hpp"
#include "thresh.hpp"
#include "types_c.h"

// clang-format off

/* initializes 8-element array for fast access to 3x3 neighborhood of a pixel */
// L42
#define  CV_INIT_3X3_DELTAS( deltas, step, nch )          \
  ((deltas)[0] =  (nch),  (deltas)[1] = -(step) + (nch),  \
   (deltas)[2] = -(step), (deltas)[3] = -(step) - (nch),  \
   (deltas)[4] = -(nch),  (deltas)[5] =  (step) - (nch),  \
   (deltas)[6] =  (step), (deltas)[7] =  (step) + (nch))

static const CvPoint icvCodeDeltas[8] =
  { CvPoint(1, 0), CvPoint(1, -1), CvPoint(0, -1), CvPoint(-1, -1), CvPoint(-1, 0), CvPoint(-1, 1), CvPoint(0, 1), CvPoint(1, 1) };

/****************************************************************************************\
*                         Raster->Chain Tree (Suzuki algorithms)                         *
\****************************************************************************************/
// L141
typedef struct _CvContourInfo {
  int flags;
  struct _CvContourInfo *next;        /* next contour with the same mark value */
  struct _CvContourInfo *parent;      /* information about parent contour */
  CvSeq *contour;             /* corresponding contour (may be 0, if rejected) */
  CvRect rect;                /* bounding rectangle */
  CvPoint origin;             /* origin point (where the contour was traced from) */
  int is_hole;                /* hole flag */
}
_CvContourInfo;

/*
  Structure that is used for sequential retrieving contours from the image.
  It supports both hierarchical and plane variants of Suzuki algorithm.
*/
// L158
typedef struct _CvContourScanner {
  CvMemStorage *storage1;     /* contains fetched contours */
  CvMemStorage *storage2;     /* contains approximated contours
                                 (!=storage1 if approx_method2 != approx_method1) */
  CvMemStorage *cinfo_storage;        /* contains _CvContourInfo nodes */
  CvSet *cinfo_set;           /* set of _CvContourInfo nodes */
  CvMemStoragePos initial_pos;        /* starting storage pos */
  CvMemStoragePos backup_pos; /* beginning of the latest approx. contour */
  CvMemStoragePos backup_pos2;        /* ending of the latest approx. contour */
  schar *img0;                /* image origin */
  schar *img;                 /* current image row */
  int img_step;               /* image step */
  CvSize img_size;            /* ROI size */
  CvPoint offset;             /* ROI offset: coordinates, added to each contour point */
  CvPoint pt;                 /* current scanner position */
  CvPoint lnbd;               /* position of the last met contour */
  int nbd;                    /* current mark val */
  _CvContourInfo *l_cinfo;    /* information about latest approx. contour */
  _CvContourInfo cinfo_temp;  /* temporary var which is used in simple modes */
  _CvContourInfo frame_info;  /* information about frame */
  CvSeq frame;                /* frame itself */
  int approx_method1;         /* approx method when tracing */
  int approx_method2;         /* final approx method */
  int mode;                   /* contour scanning mode:
                                 0 - external only
                                 1 - all the contours w/o any hierarchy
                                 2 - connected components (i.e. two-level structure -
                                 external contours and holes),
                                 3 - full hierarchy;
                                 4 - connected components of a multi-level image
                              */
  int subst_flag;
  int seq_type1;              /* type of fetched contours */
  int header_size1;           /* hdr size of fetched contours */
  int elem_size1;             /* elem size of fetched contours */
  int seq_type2;              /*                                       */
  int header_size2;           /*        the same for approx. contours  */
  int elem_size2;             /*                                       */
  _CvContourInfo *cinfo_table[128];
}
_CvContourScanner;

// clang-format on

/*
   Initializes scanner structure.
   Prepare image for scanning ( clear borders and convert all pixels to 0-1.
*/
// L208
static CvContourScanner _cvStartFindContours_Impl(void *_img, CvMemStorage *storage,
                                                  int header_size, int mode, int method,
                                                  CvPoint offset, int needFillBorder) {
  if (!storage) CV_Error(CV_StsNullPtr, "");

  CvMat stub, *mat = cvGetMat(_img, &stub);

  if (CV_MAT_TYPE(mat->type) == CV_32SC1 && mode == CV_RETR_CCOMP) mode = CV_RETR_FLOODFILL;

  if (!((CV_IS_MASK_ARR(mat) && mode < CV_RETR_FLOODFILL) ||
        (CV_MAT_TYPE(mat->type) == CV_32SC1 && mode == CV_RETR_FLOODFILL)))
    CV_Error(CV_StsUnsupportedFormat,
             "[Start]FindContours supports only CV_8UC1 images when mode != CV_RETR_FLOODFILL "
             "otherwise supports CV_32SC1 images only");

  CvSize size = cvSize(mat->width, mat->height);
  int step = mat->step;
  uchar *img = (uchar *)(mat->data.ptr);

  if (method < 0 || method > CV_CHAIN_APPROX_TC89_KCOS) CV_Error(CV_StsOutOfRange, "");

  if (header_size < (int)(method == CV_CHAIN_CODE ? sizeof(CvChain) : sizeof(CvContour)))
    CV_Error(CV_StsBadSize, "");

  CvContourScanner scanner = (CvContourScanner)cvAlloc(sizeof(*scanner));
  memset((void *)scanner, 0, sizeof(*scanner));

  scanner->storage1 = scanner->storage2 = storage;
  scanner->img0 = (schar *)img;
  scanner->img = (schar *)(img + step);
  scanner->img_step = step;
  scanner->img_size.width = size.width - 1;   /* exclude rightest column */
  scanner->img_size.height = size.height - 1; /* exclude bottomost row */
  scanner->mode = mode;
  scanner->offset = offset;
  scanner->pt.x = scanner->pt.y = 1;
  scanner->lnbd.x = 0;
  scanner->lnbd.y = 1;
  scanner->nbd = 2;
  scanner->frame_info.contour = &(scanner->frame);
  scanner->frame_info.is_hole = 1;
  scanner->frame_info.next = 0;
  scanner->frame_info.parent = 0;
  scanner->frame_info.rect = cvRect(0, 0, size.width, size.height);
  scanner->l_cinfo = 0;
  scanner->subst_flag = 0;

  scanner->frame.flags = CV_SEQ_FLAG_HOLE;

  scanner->approx_method2 = scanner->approx_method1 = method;

  if (method == CV_CHAIN_APPROX_TC89_L1 || method == CV_CHAIN_APPROX_TC89_KCOS)
    scanner->approx_method1 = CV_CHAIN_CODE;

  if (scanner->approx_method1 == CV_CHAIN_CODE) {
    scanner->seq_type1 = CV_SEQ_CHAIN_CONTOUR;
    scanner->header_size1 =
        scanner->approx_method1 == scanner->approx_method2 ? header_size : sizeof(CvChain);
    scanner->elem_size1 = sizeof(char);
  } else {
    scanner->seq_type1 = CV_SEQ_POLYGON;
    scanner->header_size1 =
        scanner->approx_method1 == scanner->approx_method2 ? header_size : sizeof(CvContour);
    scanner->elem_size1 = sizeof(CvPoint);
  }

  scanner->header_size2 = header_size;

  if (scanner->approx_method2 == CV_CHAIN_CODE) {
    scanner->seq_type2 = scanner->seq_type1;
    scanner->elem_size2 = scanner->elem_size1;
  } else {
    scanner->seq_type2 = CV_SEQ_POLYGON;
    scanner->elem_size2 = sizeof(CvPoint);
  }

  scanner->seq_type1 =
      scanner->approx_method1 == CV_CHAIN_CODE ? CV_SEQ_CHAIN_CONTOUR : CV_SEQ_POLYGON;

  scanner->seq_type2 =
      scanner->approx_method2 == CV_CHAIN_CODE ? CV_SEQ_CHAIN_CONTOUR : CV_SEQ_POLYGON;

  cvSaveMemStoragePos(storage, &(scanner->initial_pos));

  if (method > CV_CHAIN_APPROX_SIMPLE) {
    CV_Assert(0);
  }

  if (mode > CV_RETR_LIST) {
    CV_Assert(0);
  }

  CV_Assert(step >= 0);
  CV_Assert(size.height >= 1);

  /* make zero borders */
  if (needFillBorder) {
    int esz = CV_ELEM_SIZE(mat->type);
    memset(img, 0, size.width * esz);
    memset(img + static_cast<size_t>(step) * (size.height - 1), 0, size.width * esz);

    img += step;
    for (int y = 1; y < size.height - 1; y++, img += step) {
      for (int k = 0; k < esz; k++) img[k] = img[(size.width - 1) * esz + k] = (schar)0;
    }
  }

  /* converts all pixels to 0 or 1 */
  if (CV_MAT_TYPE(mat->type) != CV_32S) cvThreshold(mat, mat, 0, 1, CV_THRESH_BINARY);

  return scanner;
}

// L484
static void _icvEndProcessContour(CvContourScanner scanner) {
  _CvContourInfo *l_cinfo = scanner->l_cinfo;

  if (l_cinfo) {
    if (scanner->subst_flag) {
      CvMemStoragePos temp;

      cvSaveMemStoragePos(scanner->storage2, &temp);

      if (temp.top == scanner->backup_pos2.top &&
          temp.free_space == scanner->backup_pos2.free_space) {
        cvRestoreMemStoragePos(scanner->storage2, &scanner->backup_pos);
      }
      scanner->subst_flag = 0;
    }

    if (l_cinfo->contour) {
      cvInsertNodeIntoTree(l_cinfo->contour, l_cinfo->parent->contour, &(scanner->frame));
    }
    scanner->l_cinfo = 0;
  }
}

/*
    marks domain border with +/-<constant> and stores the contour into CvSeq.
        method:
            <0  - chain
            ==0 - direct
            >0  - simple approximation
*/
// L539
static void _icvFetchContour(schar *ptr, int step, CvPoint pt, CvSeq *contour, int _method) {
  const schar nbd = 2;
  int deltas[16];
  CvSeqWriter writer;
  schar *i0 = ptr, *i1, *i3, *i4 = 0;
  int prev_s = -1, s, s_end;
  int method = _method - 1; /* NOTE: method = 1 */

  assert((unsigned)_method <= CV_CHAIN_APPROX_SIMPLE);

  /* initialize local state */
  CV_INIT_3X3_DELTAS(deltas, step, 1);
  memcpy(deltas + 8, deltas, 8 * sizeof(deltas[0]));

  /* initialize writer */
  cvStartAppendToSeq(contour, &writer);

  if (method < 0) ((CvChain *)contour)->origin = pt;

  s_end = s = CV_IS_SEQ_HOLE(contour) ? 0 : 4;

  do {
    s = (s - 1) & 7;
    i1 = i0 + deltas[s];
  } while (*i1 == 0 && s != s_end);

  if (s == s_end) /* single pixel domain */
  {
    *i0 = (schar)(nbd | -128);
    if (method >= 0) {
      CV_WRITE_SEQ_ELEM(pt, writer);
    }
  } else {
    i3 = i0;
    prev_s = s ^ 4;

    /* follow border */
    for (;;) {
      s_end = s;

      for (;;) {
        i4 = i3 + deltas[++s];
        if (*i4 != 0) break;
      }
      s &= 7;

      /* check "right" bound */
      if ((unsigned)(s - 1) < (unsigned)s_end) {
        *i3 = (schar)(nbd | -128);
      } else if (*i3 == 1) {
        *i3 = nbd;
      }

      if (method < 0) {
        CV_Assert(0);
      } else {
        if (s != prev_s || method == 0) {
          CV_WRITE_SEQ_ELEM(pt, writer);
          prev_s = s;
        }

        pt.x += icvCodeDeltas[s].x;
        pt.y += icvCodeDeltas[s].y;
      }

      if (i4 == i0 && i3 == i1) break;

      i3 = i4;
      s = (s + 4) & 7;
    } /* end of border following loop */
  }

  cvEndWriteSeq(&writer);

  if (_method != CV_CHAIN_CODE) {
    cvBoundingRect(contour, 1);
  }

  assert((writer.seq->total == 0 && writer.seq->first == 0) ||
         writer.seq->total > writer.seq->first->count ||
         (writer.seq->first->prev == writer.seq->first &&
          writer.seq->first->next == writer.seq->first));
}

// L1018
CvSeq *_cvFindNextContour(CvContourScanner scanner) {
  if (!scanner) CV_Error(CV_StsNullPtr, "");

#if CV_SSE2
#endif
  CV_Assert(scanner->img_step >= 0);

  _icvEndProcessContour(scanner);

  /* initialize local state */
  schar *img0 = scanner->img0;
  schar *img = scanner->img;
  int step = scanner->img_step;
  // int step_i = step / sizeof(int);     /* not used */
  int x = scanner->pt.x;
  int y = scanner->pt.y;
  int width = scanner->img_size.width;
  int height = scanner->img_size.height;
  int mode = scanner->mode;
  CvPoint lnbd = scanner->lnbd;
  int nbd = scanner->nbd;
  int prev = img[x - 1];
  int new_mask = -2;

  if (mode == CV_RETR_FLOODFILL) {
    CV_Assert(0);
  }

  // bool _tmp_show_1 = false;            /* not used */

  for (; y < height; y++, img += step) {
    // int* img0_i = 0;                   /* not used */
    int *img_i = 0;
    int p = 0;

    if (mode == CV_RETR_FLOODFILL) {
      CV_Assert(0);
    }

    for (; x < width; x++) {
      if (img_i) {
        for (; x < width && ((p = img_i[x]) == prev || (p & ~new_mask) == (prev & ~new_mask)); x++)
          prev = p;
      } else {
#if CV_SSE2
#endif
        for (; x < width && (p = img[x]) == prev; x++)
          ;
      }

      if (x >= width) break;
#if CV_SSE2
#endif
      {
        _CvContourInfo *par_info = 0;
        _CvContourInfo *l_cinfo = 0;
        CvSeq *seq = 0;
        int is_hole = 0;
        CvPoint origin;

        /* if not external contour */
        if ((!img_i && !(prev == 0 && p == 1)) ||
            (img_i && !(((prev & new_mask) != 0 || prev == 0) && (p & new_mask) == 0))) {
          /* check hole */
          if ((!img_i && (p != 0 || prev < 1)) ||
              (img_i && ((prev & new_mask) != 0 || (p & new_mask) != 0)))
            goto resume_scan;

          if (prev & new_mask) {
            lnbd.x = x - 1;
          }
          is_hole = 1;
        }

        if (mode == 0 && (is_hole || img0[lnbd.y * static_cast<size_t>(step) + lnbd.x] > 0))
          goto resume_scan;

        origin.y = y;
        origin.x = x - is_hole;

        /* find contour parent */
        if (mode <= 1 || (!is_hole && (mode == CV_RETR_CCOMP || mode == CV_RETR_FLOODFILL)) ||
            lnbd.x <= 0) {
          par_info = &(scanner->frame_info);
        } else {
          CV_Assert(0);
        }

        lnbd.x = x - is_hole;

        cvSaveMemStoragePos(scanner->storage2, &(scanner->backup_pos));

        seq = cvCreateSeq(scanner->seq_type1, scanner->header_size1, scanner->elem_size1,
                          scanner->storage1);
        seq->flags |= is_hole ? CV_SEQ_FLAG_HOLE : 0;

        /* initialize header */
        if (mode <= 1) {
          l_cinfo = &(scanner->cinfo_temp);
          _icvFetchContour(img + x - is_hole, step,
                           cvPoint(origin.x + scanner->offset.x, origin.y + scanner->offset.y), seq,
                           scanner->approx_method1);
        } else {
          CV_Assert(0);
        }

        l_cinfo->is_hole = is_hole;
        l_cinfo->contour = seq;
        l_cinfo->origin = origin;
        l_cinfo->parent = par_info;

        if (scanner->approx_method1 != scanner->approx_method2) {
          CV_Assert(0);
        }

        l_cinfo->contour->v_prev = l_cinfo->parent->contour;

        if (par_info->contour == 0) {
          l_cinfo->contour = 0;
          if (scanner->storage1 == scanner->storage2) {
            cvRestoreMemStoragePos(scanner->storage1, &(scanner->backup_pos));
          } else {
            cvClearMemStorage(scanner->storage1);
          }
          p = img[x];
          goto resume_scan;
        }

        cvSaveMemStoragePos(scanner->storage2, &(scanner->backup_pos2));
        scanner->l_cinfo = l_cinfo;
        scanner->pt.x = !img_i ? x + 1 : x + 1 - is_hole;
        scanner->pt.y = y;
        scanner->lnbd = lnbd;
        scanner->img = (schar *)img;
        scanner->nbd = nbd;
        return l_cinfo->contour;

      resume_scan:

        prev = p;
        /* update lnbd */
        if (prev & -2) {
          lnbd.x = x;
        }
      } /* end of prev != p */
    }   /* end of loop on x */

    lnbd.x = 0;
    lnbd.y = y + 1;
    x = 1;
    prev = 0;
  } /* end of loop on y */

  return 0;
}

/*
   The function add to tree the last retrieved/substituted contour,
   releases temp_storage, restores state of dst_storage (if needed), and
   returns pointer to root of the contour tree */
// L1331
CV_IMPL CvSeq *_cvEndFindContours(CvContourScanner *_scanner) {
  CvContourScanner scanner;
  CvSeq *first = 0;

  if (!_scanner) CV_Error(CV_StsNullPtr, "");
  scanner = *_scanner;

  if (scanner) {
    _icvEndProcessContour(scanner);

    if (scanner->storage1 != scanner->storage2) cvReleaseMemStorage(&(scanner->storage1));

    if (scanner->cinfo_storage) cvReleaseMemStorage(&(scanner->cinfo_storage));

    first = scanner->frame.v_next;
    cvFree(_scanner);
  }

  return first;
}

// L1365
typedef struct CvLinkedRunPoint {
  struct CvLinkedRunPoint *link;
  struct CvLinkedRunPoint *next;
  CvPoint pt;
} CvLinkedRunPoint;

// L1816
static int _cvFindContours_Impl(void *img, CvMemStorage *storage, CvSeq **firstContour,
                                int cntHeaderSize, int mode, int method, CvPoint offset,
                                int needFillBorder) {
  CvContourScanner scanner = 0;
  CvSeq *contour = 0;
  int count = -1;

  if (!firstContour) CV_Error(CV_StsNullPtr, "NULL double CvSeq pointer");

  *firstContour = 0;

  if (method == CV_LINK_RUNS) {
    CV_Assert(0);
  } else {
    try {
      scanner = _cvStartFindContours_Impl(img, storage, cntHeaderSize, mode, method, offset,
                                          needFillBorder);
      do {
        count++;
        contour = _cvFindNextContour(scanner);
      } while (contour != 0);
    } catch (...) {
      if (scanner) _cvEndFindContours(&scanner);
      throw;
    }

    *firstContour = _cvEndFindContours(&scanner);
  }

  return count;
}

// L1895
void _findContours(cv::InputOutputArray _image, cv::OutputArrayOfArrays _contours,
                   cv::OutputArray _hierarchy, int mode, int method, cv::Point offset) {
  // Sanity check: output must be of type vector<vector<Point>>
  CV_Assert((_contours.kind() == cv::_InputArray::STD_VECTOR_VECTOR ||
             _contours.kind() == cv::_InputArray::STD_VECTOR_MAT ||
             _contours.kind() == cv::_InputArray::STD_VECTOR_UMAT));

  CV_Assert(_contours.empty() || (_contours.channels() == 2 && _contours.depth() == CV_32S));

  cv::Mat image = _image.getMatRef();
  // copyMakeBorder(_image, image, 1, 1, 1, 1, cv::BORDER_CONSTANT | cv::BORDER_ISOLATED,
  //                cv::Scalar(0));

  cv::MemStorage storage(cvCreateMemStorage());
  CvMat _cimage = image;
  CvSeq *_ccontours = 0;
  if (_hierarchy.needed()) _hierarchy.clear();
  _cvFindContours_Impl(&_cimage, storage, &_ccontours, sizeof(CvContour), mode, method,
                       offset + cv::Point(-1, -1), 0);
  if (!_ccontours) {
    _contours.clear();
    return;
  }
  cv::Seq<CvSeq *> all_contours(cvTreeToNodeSeq(_ccontours, sizeof(CvSeq), storage));
  int i, total = (int)all_contours.size();
  _contours.create(total, 1, 0, -1, true);
  cv::SeqIterator<CvSeq *> it = all_contours.begin();
  for (i = 0; i < total; i++, ++it) {
    CvSeq *c = *it;
    ((CvContour *)c)->color = (int)i;
    _contours.create((int)c->total, 1, CV_32SC2, i, true);
    cv::Mat ci = _contours.getMat(i);
    CV_Assert(ci.isContinuous());
    cvCvtSeqToArray(c, ci.ptr());
  }

  if (_hierarchy.needed()) {
    _hierarchy.create(1, total, CV_32SC4, -1, true);
    cv::Vec4i *hierarchy = _hierarchy.getMat().ptr<cv::Vec4i>();

    it = all_contours.begin();
    for (i = 0; i < total; i++, ++it) {
      CvSeq *c = *it;
      int h_next = c->h_next ? ((CvContour *)c->h_next)->color : -1;
      int h_prev = c->h_prev ? ((CvContour *)c->h_prev)->color : -1;
      int v_next = c->v_next ? ((CvContour *)c->v_next)->color : -1;
      int v_prev = c->v_prev ? ((CvContour *)c->v_prev)->color : -1;
      hierarchy[i] = cv::Vec4i(h_next, h_prev, v_next, v_prev);
    }
  }
}

// L1951
void cvitdl::findContours(cv::InputOutputArray _image, cv::OutputArrayOfArrays _contours, int mode,
                          int method, cv::Point offset) {
  /* NOTE: mode = RETR_EXTERNAL, method = CHAIN_APPROX_SIMPLE */
  _findContours(_image, _contours, cv::noArray(), mode, method, offset);
}