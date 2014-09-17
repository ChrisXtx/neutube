#ifndef ZSTACKOPERATOR_H
#define ZSTACKOPERATOR_H

#include "swctreenode.h"
#include "zmouseevent.h"

class ZMouseEventRecorder;

class ZStackOperator
{
public:
  ZStackOperator();

  enum EOperation {
    OP_NULL, OP_MOVE_IMAGE, OP_MOVE_OBJECT, OP_CAPTURE_MOUSE_POSITION,
    OP_PROCESS_OBJECT, OP_RESOTRE_EXPLORE_MODE, OP_TRACK_MOUSE_MOVE,
    OP_PAINT_STROKE, OP_START_MOVE_IMAGE, OP_SHOW_STACK_CONTEXT_MENU,
    OP_SHOW_SWC_CONTEXT_MENU, OP_SHOW_STROKE_CONTEXT_MENU,
    OP_SHOW_PUNCTA_CONTEXT_MENU,  OP_SHOW_TRACE_CONTEXT_MENU,
    OP_SHOW_PUNCTA_MENU,
    OP_EXIT_EDIT_MODE,
    OP_SWC_SELECT, OP_SWC_SELECT_SINGLE_NODE, OP_SWC_SELECT_MULTIPLE_NODE,
    OP_SWC_DESELECT_SINGLE_NODE, OP_SWC_DESELECT_ALL_NODE,
    OP_SWC_EXTEND, OP_SWC_SMART_EXTEND, OP_SWC_CONNECT, OP_SWC_ADD_NODE,
    OP_SWC_DELETE_NODE, OP_SWC_ELECT_ALL_NODE,
    OP_SWC_MOVE_NODE_LEFT, OP_SWC_MOVE_NODE_LEFT_FAST,
    OP_SWC_MOVE_NODE_RIGHT, OP_SWC_MOVE_NODE_RIGHT_FAST,
    OP_SWC_MOVE_NODE_UP, OP_SWC_MOVE_NODE_UP_FAST,
    OP_SWC_MOVE_NODE_DOWN, OP_SWC_MOVE_NODE_DOWN_FAST,
    OP_SWC_INCREASE_NODE_SIZE, OP_SWC_DECREASE_NODE_SIZE,
    OP_SWC_CONNECT_NODE, OP_SWC_CONNECT_NODE_SMART,
    OP_SWC_BREAK_NODE, OP_SWC_CONNECT_ISOLATE,
    OP_SWC_ZOOM_TO_SELECTED_NODE, OP_SWC_INSERT_NODE, OP_SWC_MOVE_NODE,
    OP_SWC_RESET_BRANCH_POINT, OP_SWC_CHANGE_NODE_FOCUS,
    OP_SWC_SELECT_CONNECTION,
    OP_SWC_SELECT_FLOOD, OP_SWC_CONNECT_TO,
    OP_SWC_LOCATE_FOCUS,
    OP_PUNCTA_SELECT_SINGLE, OP_PUNCTA_SELECT_MULTIPLE,
    OP_STROKE_ADD_NEW, OP_STROKE_START_PAINT,
    OP_STACK_LOCATE_SLICE, OP_STACK_VIEW_PROJECTION,
    OP_STACK_VIEW_SLICE
  };

  inline EOperation getOperation() const {
    return m_op;
  }

  inline void setOperation(EOperation op) {
    m_op = op;
  }

  inline void setMouseEventRecorder(const ZMouseEventRecorder *recorder) {
    m_mouseEventRecorder = recorder;
  }

  inline Swc_Tree_Node* getHitSwcNode() const {
    return m_hitNode;
  }

  inline void setHitSwcNode(Swc_Tree_Node *tn) {
    m_hitNode = tn;
  }

  inline void setHitPuncta(int index) {
    m_punctaIndex = index;
  }

  inline int getPunctaIndex() const {
    return m_punctaIndex;
  }

  bool isNull() const;

  ZPoint getMouseOffset(ZMouseEvent::ECoordinateSystem cs) const;
  inline const ZMouseEventRecorder* getMouseEventRecorder() const {
    return m_mouseEventRecorder;
  }

private:
  EOperation m_op;
  Swc_Tree_Node *m_hitNode;
  int m_punctaIndex;
  const ZMouseEventRecorder *m_mouseEventRecorder;
};

#endif // ZSTACKOPERATOR_H