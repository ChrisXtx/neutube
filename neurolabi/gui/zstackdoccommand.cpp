#include "zstackdoccommand.h"

#include <iostream>
#include <QMutableListIterator>
#include <QMessageBox>
#include <QApplication>

#include "zswctree.h"
#include "tz_error.h"
#include "zlocsegchainconn.h"
#include "zlocsegchain.h"
#include "zstack.hxx"
#include "zstroke2d.h"
#include "zneurontracer.h"
#include "zstackdoc.h"
#include "zswcconnector.h"
#include "zgraph.h"
#include "zdocumentable.h"
#include "zdocplayer.h"
#include "neutubeconfig.h"

using namespace std;

#define INIT_ZUNDOCOMMAND m_isSwcSaved(false)

ZUndoCommand::ZUndoCommand(QUndoCommand *parent) : QUndoCommand(parent),
  INIT_ZUNDOCOMMAND
{

}

ZUndoCommand::ZUndoCommand(const QString &text, QUndoCommand *parent) :
  QUndoCommand(text, parent), INIT_ZUNDOCOMMAND
{

}

void ZUndoCommand::setSaved(NeuTube::EDocumentableType type, bool state)
{
  switch (type) {
  case NeuTube::Documentable_SWC:
    m_isSwcSaved = state;
    break;
  default:
    break;
  }
}

bool ZUndoCommand::isSaved(NeuTube::EDocumentableType type) const
{
  switch (type) {
  case NeuTube::Documentable_SWC:
    return m_isSwcSaved;
  default:
    return false;
  }

  return false;
}

ZStackDocCommand::SwcEdit::TranslateRoot::TranslateRoot(
    ZStackDoc *doc, double x, double y, double z, QUndoCommand *parent)
  :ZUndoCommand(parent), m_doc(doc), m_x(x), m_y(y), m_z(z)
{
  setText(QObject::tr("translate swc tree root to (%1,%2,%3)").
          arg(m_x).arg(m_y).arg(m_z));
}

ZStackDocCommand::SwcEdit::TranslateRoot::~TranslateRoot()
{
  for (int i=0; i<m_swcList.size(); i++) {
    delete m_swcList[i];
  }
}

void ZStackDocCommand::SwcEdit::TranslateRoot::undo()
{
  m_doc->blockSignals(true);
  for (int i = 0; i < m_doc->getSwcList().size(); i++) {
    m_doc->removeObject(m_doc->getSwcTree(i), true);
  }
  for (int i=0; i<m_swcList.size(); i++) {
    m_doc->addSwcTree(m_swcList[i]);
  }
  m_swcList.clear();
  m_doc->updateVirtualStackSize();
  m_doc->blockSignals(false);
  m_doc->notifySwcModified();
}

void ZStackDocCommand::SwcEdit::TranslateRoot::redo()
{
  m_doc->blockSignals(true);
  m_swcList.clear();
  for (int i=0; i<m_doc->getSwcList().size(); i++) {
    ZSwcTree *doctree = m_doc->getSwcList()[i]->clone();
    m_swcList.push_back(doctree);
  }
  m_doc->swcTreeTranslateRootTo(m_x, m_y, m_z);
  m_doc->updateVirtualStackSize();
  m_doc->blockSignals(false);
  m_doc->notifySwcModified();
}

ZStackDocCommand::SwcEdit::Rescale::Rescale(
    ZStackDoc *doc, double scaleX, double scaleY, double scaleZ, QUndoCommand *parent)
  :ZUndoCommand(parent), m_doc(doc), m_scaleX(scaleX), m_scaleY(scaleY), m_scaleZ(scaleZ)
{
  setText(QObject::tr("rescale swc tree (%1,%2,%3)").arg(scaleX).arg(scaleY).arg(scaleZ));
}

ZStackDocCommand::SwcEdit::Rescale::Rescale(
    ZStackDoc *doc, double srcPixelPerUmXY, double srcPixelPerUmZ,
    double dstPixelPerUmXY, double dstPixelPerUmZ, QUndoCommand *parent)
  :ZUndoCommand(parent), m_doc(doc)
{
  m_scaleX = dstPixelPerUmXY/srcPixelPerUmXY;
  m_scaleY = m_scaleX;
  m_scaleZ = dstPixelPerUmZ/srcPixelPerUmZ;
  setText(QObject::tr("rescale swc tree (%1,%2,%3)").
          arg(m_scaleX).arg(m_scaleY).arg(m_scaleZ));
}

ZStackDocCommand::SwcEdit::Rescale::~Rescale()
{
  for (int i=0; i<m_swcList.size(); i++) {
    delete m_swcList[i];
  }
}

void ZStackDocCommand::SwcEdit::Rescale::undo()
{
  m_doc->blockSignals(true);

  m_doc->removeAllSwcTree(true);

//  for (QList<ZSwcTree*>::iterator iter = m_doc->getSwcIteratorBegin();
//       iter != m_doc->getSwcIteratorEnd(); ++iter) {
//    m_doc->removeObject(*iter, true);
//  }

  m_doc->addSwcTree(m_swcList);

  m_swcList.clear();
  m_doc->updateVirtualStackSize();
  m_doc->blockSignals(false);
  m_doc->notifySwcModified();
}

void ZStackDocCommand::SwcEdit::Rescale::redo()
{
  m_doc->blockSignals(true);
  m_swcList.clear();

  QList<ZSwcTree*> swcList = m_doc->getSwcList();
  for (QList<ZSwcTree*>::iterator iter = swcList.begin();
       iter != swcList.end(); ++iter) {
    ZSwcTree *doctree = (*iter)->clone();
    m_swcList.push_back(doctree);
  }

  m_doc->swcTreeRescale(m_scaleX, m_scaleY, m_scaleZ);
  m_doc->updateVirtualStackSize();
  m_doc->blockSignals(false);
  m_doc->notifySwcModified();
}

ZStackDocCommand::SwcEdit::RescaleRadius::RescaleRadius(
    ZStackDoc *doc, double scale, int startdepth, int enddepth, QUndoCommand *parent)
  :ZUndoCommand(parent), m_doc(doc), m_scale(scale), m_startdepth(startdepth),
    m_enddepth(enddepth)
{
  if (enddepth < 0) {
    setText(QObject::tr("rescale radius of swc nodes with depth in [%1,max) by %2").
            arg(startdepth).arg(scale));
  } else {
    setText(QObject::tr("rescale radius of swc nodes with depth in [%1,%2) by %3").
            arg(startdepth).arg(enddepth).arg(scale));
  }
}

ZStackDocCommand::SwcEdit::RescaleRadius::~RescaleRadius()
{
  for (int i=0; i<m_swcList.size(); i++) {
    delete m_swcList[i];
  }
}

void ZStackDocCommand::SwcEdit::RescaleRadius::undo()
{
  m_doc->blockSignals(true);
  m_doc->removeAllSwcTree(true);
  m_doc->addSwcTree(m_swcList);
  m_swcList.clear();
  m_doc->updateVirtualStackSize();
  m_doc->blockSignals(false);

  m_doc->notifySwcModified();
}

void ZStackDocCommand::SwcEdit::RescaleRadius::redo()
{
  m_doc->blockSignals(true);
  m_swcList.clear();
  QList<ZSwcTree*> swcList = m_doc->getSwcList();
  for (QList<ZSwcTree*>::const_iterator iter = swcList.begin();
       iter != swcList.end(); ++iter) {
    ZSwcTree *doctree = (*iter)->clone();
    m_swcList.push_back(doctree);
  }
  m_doc->swcTreeRescaleRadius(m_scale, m_startdepth, m_enddepth);
  m_doc->updateVirtualStackSize();
  m_doc->blockSignals(false);

  m_doc->notifySwcModified();
}

ZStackDocCommand::SwcEdit::ReduceNodeNumber::ReduceNodeNumber(
    ZStackDoc *doc, double lengthThre, QUndoCommand *parent)
  :ZUndoCommand(parent), m_doc(doc), m_lengthThre(lengthThre)
{
  setText(QObject::tr("reduce number of swc node use length thre %1").arg(lengthThre));
}

ZStackDocCommand::SwcEdit::ReduceNodeNumber::~ReduceNodeNumber()
{
  for (int i=0; i<m_swcList.size(); i++) {
    delete m_swcList[i];
  }
}

void ZStackDocCommand::SwcEdit::ReduceNodeNumber::undo()
{
  m_doc->blockSignals(true);
  m_doc->removeAllSwcTree(true);
  m_doc->addSwcTree(m_swcList);
  m_swcList.clear();
  m_doc->blockSignals(false);

  m_doc->notifySwcModified();
}

void ZStackDocCommand::SwcEdit::ReduceNodeNumber::redo()
{
  m_doc->blockSignals(true);
  m_swcList.clear();
  QList<ZSwcTree*> swcList = m_doc->getSwcList();
  for (QList<ZSwcTree*>::const_iterator iter = swcList.begin();
       iter != swcList.end(); ++iter) {
    ZSwcTree *doctree = (*iter)->clone();
    m_swcList.push_back(doctree);
  }
  m_doc->swcTreeReduceNodeNumber(m_lengthThre);
  m_doc->blockSignals(false);

  m_doc->notifySwcModified();
}

ZStackDocCommand::SwcEdit::AddSwc::AddSwc(
    ZStackDoc *doc, ZSwcTree *tree, QUndoCommand *parent) :
  ZUndoCommand(parent), m_doc(doc), m_tree(tree), m_isInDoc(false)
{
  setText(QObject::tr("Add swc"));
}

ZStackDocCommand::SwcEdit::AddSwc::~AddSwc()
{
#ifdef _DEBUG_
  std::cout << "ZStackDocCommand::SwcEdit::AddSwc destroyed" << std::endl;
#endif
  if (!m_isInDoc) {
    delete m_tree;
  }
}

void ZStackDocCommand::SwcEdit::AddSwc::redo()
{
  m_doc->addSwcTree(m_tree, false);
  m_isInDoc = true;
  m_doc->notifySwcModified();
}

void ZStackDocCommand::SwcEdit::AddSwc::undo()
{
  m_doc->removeObject(m_tree, false);
  m_isInDoc = false;
  m_doc->notifySwcModified();
}

int ZStackDocCommand::SwcEdit::AddSwcNode::m_index = 1;

ZStackDocCommand::SwcEdit::AddSwcNode::AddSwcNode(
    ZStackDoc *doc, Swc_Tree_Node *tn, QUndoCommand *parent)
  : ZUndoCommand(parent), m_doc(doc), m_node(tn), m_treeInDoc(false)
{
  setText(QObject::tr("Add Neuron Node"));
  m_tree = new ZSwcTree();
  if (doc->getTag() == NeuTube::Document::FLYEM_ROI) {
    m_tree->useCosmeticPen(true);
    m_tree->setStructrualMode(ZSwcTree::STRUCT_CLOSED_CURVE);
  }

  m_tree->setDataFromNode(m_node);
  m_tree->setSource(QString("#added by add neuron node command %1").
                    arg(m_index++).toStdString());
}

ZStackDocCommand::SwcEdit::AddSwcNode::~AddSwcNode()
{
  if (!m_treeInDoc) {
    delete m_tree;
  }
#ifdef _DEBUG_2
  std::cout << "Command AddSwcNode destroyed" << endl;
#endif
}

void ZStackDocCommand::SwcEdit::AddSwcNode::undo()
{
  m_doc->blockSignals(true);
  m_doc->removeObject(m_tree);
  m_doc->setSwcSelected(m_tree, false);
  m_doc->deselectSwcTreeNode(m_node);
  //m_doc->setSwcTreeNodeSelected(m_node, false);
  m_doc->blockSignals(false);
  m_treeInDoc = false;
  m_doc->notifySwcModified();
}

void ZStackDocCommand::SwcEdit::AddSwcNode::redo()
{
  if (m_doc->getTag() == NeuTube::Document::FLYEM_ROI) {
    m_doc->addObject(m_tree, NeuTube::Documentable_SWC, ZDocPlayer::ROLE_ROI);
  } else {
    m_doc->addSwcTree(m_tree);
  }
  m_treeInDoc = true;
}

ZStackDocCommand::SwcEdit::RemoveSubtree::RemoveSubtree(
    ZStackDoc *doc, Swc_Tree_Node *node, QUndoCommand *parent) :
  CompositeCommand(doc, parent)
{
  new SetParent(doc, node, NULL);
}

ZStackDocCommand::SwcEdit::RemoveSubtree::~RemoveSubtree()
{
#ifdef _DEBUG_
  std::cout << "RemoveSubtree destroyed." << std::endl;
#endif

  if (m_node != NULL) {
    SwcTreeNode::killSubtree(m_node);
  }
}


ZStackDocCommand::SwcEdit::MergeSwcNode::MergeSwcNode(
    ZStackDoc *doc, QUndoCommand *parent) : CompositeCommand(doc, parent)
{
  setText(QObject::tr("Merge swc nodes"));

  Swc_Tree_Node *coreNode = NULL;

  std::set<Swc_Tree_Node*> nodeSet = doc->getSelectedSwcNodeSet();
  if (nodeSet.size() > 1) {
    ZPoint center = SwcTreeNode::centroid(nodeSet);
    double radius = SwcTreeNode::maxRadius(nodeSet);

    coreNode = SwcTreeNode::makePointer(center, radius);
#ifdef _DEBUG_
    std::cout << coreNode << " created." << std::endl;
#endif

    set<Swc_Tree_Node*> parentSet;
    //set<Swc_Tree_Node*> childSet;

    for (set<Swc_Tree_Node*>::iterator iter = nodeSet.begin();
         iter != nodeSet.end(); ++iter) {
      TZ_ASSERT(*iter != NULL, "Null swc node");

      if (SwcTreeNode::isRegular(SwcTreeNode::parent(*iter))) {
        if (nodeSet.count(SwcTreeNode::parent(*iter)) == 0) {
          parentSet.insert(parentSet.end(), SwcTreeNode::parent(*iter));
        }
      }

      Swc_Tree_Node *child = SwcTreeNode::firstChild(*iter);
      while (child != NULL) {
        Swc_Tree_Node *nextChild = SwcTreeNode::nextSibling(child);
        if (nodeSet.count(child) == 0) {
          new SwcEdit::SetParent(m_doc, child, coreNode, this);
          //SwcTreeNode::setParent(child, coreNode);
          //childSet.insert(childSet.end(), child);
        }
        child = nextChild;
      }
    }

    if (parentSet.empty()) { //try to attach to a virtual root
      for (set<Swc_Tree_Node*>::iterator iter = nodeSet.begin();
           iter != nodeSet.end(); ++iter) {
        if (SwcTreeNode::isVirtual(SwcTreeNode::parent(*iter))) {
          new SwcEdit::SetParent(
                m_doc, coreNode, SwcTreeNode::parent(*iter), this);
          //SwcTreeNode::setParent(coreNode, parent(*iter));
          break;
        }
      }
    } else if (parentSet.size() > 1) {
      for (set<Swc_Tree_Node*>::iterator iter = parentSet.begin();
           iter != parentSet.end(); ++iter) {
        //SwcTreeNode::setParent(*iter, coreNode);
        new SwcEdit::SetParent(m_doc, *iter, coreNode, this);
      }
    } else {
      //SwcTreeNode::setParent(coreNode, *parentSet.begin());
      new SwcEdit::SetParent(m_doc, coreNode, *parentSet.begin(), this);
    }

    for (set<Swc_Tree_Node*>::iterator
         iter = nodeSet.begin();
         iter != nodeSet.end(); ++iter) {
      //new SwcEdit::SetParent(m_doc, *iter, NULL, this);
      if (SwcTreeNode::parent(*iter) != NULL) { //orphan node aready handled by SetParent
        new SwcEdit::DeleteSwcNode(m_doc, *iter, SwcTreeNode::root(*iter), this);
      }
    }

    m_doc->blockSignals(true);
    m_doc->deselectAllSwcTreeNodes();
    m_doc->blockSignals(false);
    //m_doc->selectedSwcTreeNodes()->clear();
    if (coreNode != NULL) {
      m_doc->selectSwcTreeNode(coreNode);
    }
  }
}

ZStackDocCommand::SwcEdit::MergeSwcNode::~MergeSwcNode()
{
#ifdef _DEBUG_
    std::cout << "MergeSwcNode destroyed" << std::endl;
#endif
}

ZStackDocCommand::SwcEdit::ExtendSwcNode::ExtendSwcNode(
    ZStackDoc *doc, Swc_Tree_Node *node, Swc_Tree_Node *pnode,
    QUndoCommand *parent)
  : ZUndoCommand(parent), m_doc(doc), m_node(node), m_parentNode(pnode),
    m_nodeInDoc(false)
{
  setText(QObject::tr("Extend Selected Swc Node"));
}

ZStackDocCommand::SwcEdit::ExtendSwcNode::~ExtendSwcNode()
{
  if (!m_nodeInDoc) {
#ifdef _DEBUG_
    std::cout << "ExtendSwcNode destroyed" << std::endl;
#endif
    SwcTreeNode::kill(m_node);
  }
}

void ZStackDocCommand::SwcEdit::ExtendSwcNode::undo()
{
  // after undo, m_parentNode should be the only selected node
  m_doc->blockSignals(true);
  SwcTreeNode::detachParent(m_node);
  m_doc->deselectAllSwcTreeNodes();
  m_doc->selectSwcTreeNode(m_parentNode);
  m_doc->blockSignals(false);
  m_doc->notifySwcModified();
  m_nodeInDoc = false;
}

void ZStackDocCommand::SwcEdit::ExtendSwcNode::redo()
{
  // after redo, m_node will be the only selected node, this allows continuous extending
  m_doc->blockSignals(true);
  SwcTreeNode::setParent(m_node, m_parentNode);
  m_doc->deselectAllSwcTreeNodes();
  m_doc->selectSwcTreeNode(m_node);
  m_doc->blockSignals(false);
  m_doc->notifySwcModified();
  m_nodeInDoc = true;
}

ZStackDocCommand::SwcEdit::ChangeSwcNodeGeometry::ChangeSwcNodeGeometry(
    ZStackDoc *doc, Swc_Tree_Node *node, double x, double y, double z, double r,
    QUndoCommand *parent) :
  ZUndoCommand(parent), m_doc(doc), m_node(node), m_x(x), m_y(y), m_z(z), m_r(r),
  m_backupX(0.0), m_backupY(0.0), m_backupZ(0.0), m_backupR(0.0)
{
  setText(QObject::tr("Change geometry of Selected Swc Node"));
}

ZStackDocCommand::SwcEdit::ChangeSwcNodeGeometry::~ChangeSwcNodeGeometry()
{

}

void ZStackDocCommand::SwcEdit::ChangeSwcNodeGeometry::redo()
{
  if (m_node != NULL) {
    m_backupX = SwcTreeNode::x(m_node);
    m_backupY = SwcTreeNode::y(m_node);
    m_backupZ = SwcTreeNode::z(m_node);
    m_backupR = SwcTreeNode::radius(m_node);
    SwcTreeNode::setPos(m_node, m_x, m_y, m_z);
    SwcTreeNode::setRadius(m_node, m_r);
    m_doc->notifySwcModified();
  }
}

void ZStackDocCommand::SwcEdit::ChangeSwcNodeGeometry::undo()
{
  if (m_node != NULL) {
    SwcTreeNode::setPos(m_node, m_backupX, m_backupY, m_backupZ);
    SwcTreeNode::setRadius(m_node, m_backupR);
    m_doc->notifySwcModified();
  }
}

ZStackDocCommand::SwcEdit::ChangeSwcNodeZ::ChangeSwcNodeZ(
    ZStackDoc *doc, Swc_Tree_Node *node, double z, QUndoCommand *parent) :
  ZUndoCommand(parent), m_doc(doc), m_node(node), m_z(z), m_backup(0.0)
{
  setText(QObject::tr("Change Z of Selected Swc Node"));
}

ZStackDocCommand::SwcEdit::ChangeSwcNodeZ::~ChangeSwcNodeZ()
{

}

void ZStackDocCommand::SwcEdit::ChangeSwcNodeZ::redo()
{
  if (m_node != NULL) {
    m_backup = SwcTreeNode::z(m_node);
    SwcTreeNode::setZ(m_node, m_z);
    m_doc->notifySwcModified();
  }
}

void ZStackDocCommand::SwcEdit::ChangeSwcNodeZ::undo()
{
  if (m_node != NULL) {
    SwcTreeNode::setZ(m_node, m_backup);
    m_doc->notifySwcModified();
  }
}

//////////////////
ZStackDocCommand::SwcEdit::ChangeSwcNodeRadius::ChangeSwcNodeRadius(
    ZStackDoc *doc, Swc_Tree_Node *node, double radius, QUndoCommand *parent) :
  ZUndoCommand(parent), m_doc(doc), m_node(node), m_radius(radius), m_backup(0.0)
{
  setText(QObject::tr("Change Radius of Selected Swc Node"));
}

ZStackDocCommand::SwcEdit::ChangeSwcNodeRadius::~ChangeSwcNodeRadius()
{

}

void ZStackDocCommand::SwcEdit::ChangeSwcNodeRadius::redo()
{
  if (m_node != NULL) {
    m_backup = SwcTreeNode::radius(m_node);
    SwcTreeNode::setRadius(m_node, m_radius);
    m_doc->notifySwcModified();
  }
}

void ZStackDocCommand::SwcEdit::ChangeSwcNodeRadius::undo()
{
  if (m_node != NULL) {
    SwcTreeNode::setRadius(m_node, m_backup);
    m_doc->notifySwcModified();
  }
}


/////////////////////////

ZStackDocCommand::SwcEdit::ChangeSwcNode::ChangeSwcNode(
    ZStackDoc *doc, Swc_Tree_Node *node, const Swc_Tree_Node &newNode,
    QUndoCommand *parent) : ZUndoCommand(parent), m_doc(doc), m_node(node)
{
  m_newNode = newNode;
}

ZStackDocCommand::SwcEdit::ChangeSwcNode::~ChangeSwcNode()
{

}

void ZStackDocCommand::SwcEdit::ChangeSwcNode::redo()
{
  if (m_node != NULL) {
    m_backup = *m_node;
    *m_node = m_newNode;
  }
}

void ZStackDocCommand::SwcEdit::ChangeSwcNode::undo()
{
  if (m_node != NULL) {
    *m_node = m_backup;
  }
}

ZStackDocCommand::SwcEdit::DeleteSwcNode::DeleteSwcNode(
    ZStackDoc *doc, Swc_Tree_Node *node, Swc_Tree_Node *root,
    QUndoCommand *parent) :
  ZUndoCommand(parent), m_doc(doc), m_node(node), m_root(root),
  m_prevSibling(NULL), m_lastChild(NULL), m_nodeInDoc(true)
{
  TZ_ASSERT(m_root != NULL, "Null root");
  setText(QObject::tr("Delete swc node"));

  SwcTreeNode::setDefault(&m_backup);
}

ZStackDocCommand::SwcEdit::DeleteSwcNode::~DeleteSwcNode()
{
  if (!m_nodeInDoc) {
#ifdef _DEBUG_
    std::cout << "DeleteSwcNode destroyed" << std::endl;
#endif
    SwcTreeNode::kill(m_node);
  }
}

void ZStackDocCommand::SwcEdit::DeleteSwcNode::redo()
{
  // ignore virtual node otherwise destructor of RemoveEmptyTree will crash
  if (SwcTreeNode::parent(m_node) == NULL)
    return;
  //backup
  m_backup = *m_node;
  m_prevSibling = SwcTreeNode::prevSibling(m_node);
  m_lastChild = SwcTreeNode::lastChild(m_node);

  Swc_Tree_Node *parent = SwcTreeNode::parent(m_node);
  if (parent != NULL) {
    SwcTreeNode::detachParent(m_node);
  }

  Swc_Tree_Node *child = SwcTreeNode::firstChild(m_node);
  while (child != NULL) {
    Swc_Tree_Node *nextSibling = SwcTreeNode::nextSibling(child);
    SwcTreeNode::setParent(child, m_root);
    child = nextSibling;
  }

  m_nodeInDoc = false;
  m_doc->notifySwcModified();
}

void ZStackDocCommand::SwcEdit::DeleteSwcNode::undo()
{
  *m_node = m_backup; //forward set recovered here

  //Recover child set of the virtual root
  Swc_Tree_Node *child = SwcTreeNode::firstChild(m_node);
  SwcTreeNode::setLink(SwcTreeNode::prevSibling(child),
                       SwcTreeNode::nextSibling(m_lastChild),
                       SwcTreeNode::NEXT_SIBLING);
  SwcTreeNode::setLink(m_lastChild, NULL, SwcTreeNode::NEXT_SIBLING);

  //Recover supervisor
  if (m_prevSibling == NULL) {
    SwcTreeNode::setLink(SwcTreeNode::parent(m_node), m_node,
                         SwcTreeNode::FIRST_CHILD);
  } else {
    SwcTreeNode::setLink(m_prevSibling, m_node, SwcTreeNode::NEXT_SIBLING);
  }

  //Recover other backward links
  if (child != NULL) {
    while (child != m_lastChild) {
      SwcTreeNode::setLink(child, m_node, SwcTreeNode::PARENT);
      child = SwcTreeNode::nextSibling(child);
    }
    SwcTreeNode::setLink(child, m_node, SwcTreeNode::PARENT);
  }

  m_nodeInDoc = true;

  m_doc->notifySwcModified();
}

ZStackDocCommand::SwcEdit::CompositeCommand::CompositeCommand(
    ZStackDoc *doc, QUndoCommand *parent) : ZUndoCommand(parent), m_doc(doc)
{
}

ZStackDocCommand::SwcEdit::CompositeCommand::~CompositeCommand()
{
  qDebug() << "Composite command (" << this->text() << ") destroyed";
}

void ZStackDocCommand::SwcEdit::CompositeCommand::redo()
{
  m_doc->blockSignals(true);
  QUndoCommand::redo();
  m_doc->blockSignals(false);
  m_doc->notifySwcModified();
}


void ZStackDocCommand::SwcEdit::CompositeCommand::undo()
{
  m_doc->blockSignals(true);
  QUndoCommand::undo();
  m_doc->blockSignals(false);
  m_doc->notifySwcModified();
}

ZStackDocCommand::SwcEdit::SetParent::SetParent(
    ZStackDoc *doc, Swc_Tree_Node *node, Swc_Tree_Node *parentNode,
    QUndoCommand *parent) :
  ZUndoCommand(parent), m_doc(doc), m_node(node), m_newParent(parentNode),
  m_oldParent(NULL), m_prevSibling(NULL)
{
  //TZ_ASSERT(parentNode != NULL, "Null pointer");
#ifdef _DEBUG_
  std::cout << m_newParent << " --> " << m_node << std::endl;
#endif

  setText(QObject::tr("Set swc node parent"));
}

ZStackDocCommand::SwcEdit::SetParent::~SetParent()
{
  /*
  if (m_node != NULL) {
    if (!m_isInDoc) { //orphan node
      SwcTreeNode::killSubtree(m_node);
    }
  }
  */
}

void ZStackDocCommand::SwcEdit::SetParent::redo()
{
  m_oldParent = SwcTreeNode::parent(m_node);

#ifdef _DEBUG_
  if (m_newParent == NULL) {
    std::cout << "ZStackDocCommand::SwcEdit::SetParent : Null parent" << std::endl;
  }
#endif

  if (m_oldParent != m_newParent) {
    m_prevSibling = SwcTreeNode::prevSibling(m_node);
    SwcTreeNode::setParent(m_node, m_newParent);

    m_doc->notifySwcModified();
  }
}

void ZStackDocCommand::SwcEdit::SetParent::undo()
{
  if (m_oldParent != m_newParent) {
    //Recover child set of the new parent
    SwcTreeNode::detachParent(m_node);

    //Recover its supervisor
    Swc_Tree_Node *nextSibling = NULL;
    if (m_prevSibling == NULL) {
      nextSibling = SwcTreeNode::firstChild(m_oldParent);
      SwcTreeNode::setLink(m_oldParent, m_node,
                           SwcTreeNode::FIRST_CHILD);
    } else {
      nextSibling = SwcTreeNode::nextSibling(m_prevSibling);
      SwcTreeNode::setLink(m_prevSibling, m_node, SwcTreeNode::NEXT_SIBLING);
    }

    //Recover forward set
    SwcTreeNode::setLink(m_node, m_oldParent, SwcTreeNode::PARENT);
    SwcTreeNode::setLink(m_node, nextSibling, SwcTreeNode::NEXT_SIBLING);

    m_doc->notifySwcModified();
  }
}

////////////////////////////////
#if 1
ZStackDocCommand::SwcEdit::SetSwcNodeSeletion::SetSwcNodeSeletion(
    ZStackDoc *doc, ZSwcTree *host, const std::set<Swc_Tree_Node *> nodeSet,
    bool appending, QUndoCommand *parent) :
  ZUndoCommand(parent), m_doc(doc), m_host(host), m_nodeSet(nodeSet),
  m_appending(appending)
{
}

ZStackDocCommand::SwcEdit::SetSwcNodeSeletion::~SetSwcNodeSeletion()
{
}

void ZStackDocCommand::SwcEdit::SetSwcNodeSeletion::redo()
{
  if (m_host != NULL) {
    m_oldNodeSet = m_host->getSelectedNode();
    //m_host->deselectAllNode();
    m_host->selectNode(m_nodeSet.begin(), m_nodeSet.end(), m_appending);
    m_doc->notifySelectionAdded(m_oldNodeSet, m_nodeSet);
    m_doc->notifySelectionRemoved(m_oldNodeSet, m_nodeSet);
  }
}

void ZStackDocCommand::SwcEdit::SetSwcNodeSeletion::undo()
{
  if (m_host != NULL) {
    //m_nodeSet = m_host->getSelectedNode();
    m_host->deselectAllNode();
    m_host->selectNode(m_oldNodeSet.begin(), m_oldNodeSet.end(), true);
    m_doc->notifySelectionAdded(m_nodeSet, m_oldNodeSet);
    m_doc->notifySelectionRemoved(m_nodeSet, m_oldNodeSet);
  }
}
#endif

//////////////
ZStackDocCommand::SwcEdit::SwcTreeLabeTraceMask::SwcTreeLabeTraceMask(
    ZStackDoc *doc, Swc_Tree *tree, QUndoCommand *parent) :
  ZUndoCommand(parent), m_doc(doc), m_tree(tree)
{
}

ZStackDocCommand::SwcEdit::SwcTreeLabeTraceMask::~SwcTreeLabeTraceMask() {}

void ZStackDocCommand::SwcEdit::SwcTreeLabeTraceMask::undo()
{
  if (m_doc != NULL && m_tree != NULL) {
    Swc_Tree_Node_Label_Workspace workspace;
    Default_Swc_Tree_Node_Label_Workspace(&workspace);
    workspace.sdw.color.r = 0;
    workspace.sdw.color.g = 0;
    workspace.sdw.color.b = 0;
    Swc_Tree_Label_Stack(
          m_tree, m_doc->getTraceWorkspace()->trace_mask, &workspace);
  }
}

void ZStackDocCommand::SwcEdit::SwcTreeLabeTraceMask::redo()
{
  if (m_doc != NULL && m_tree != NULL) {
    Swc_Tree_Node_Label_Workspace workspace;
    Default_Swc_Tree_Node_Label_Workspace(&workspace);
    Swc_Tree_Label_Stack(
          m_tree, m_doc->getTraceWorkspace()->trace_mask, &workspace);
  }
}
////////////

ZStackDocCommand::SwcEdit::SwcPathLabeTraceMask::SwcPathLabeTraceMask(
    ZStackDoc *doc, const ZSwcPath &branch, QUndoCommand *parent) :
  ZUndoCommand(parent), m_doc(doc)
{
  m_branch = branch;
}

ZStackDocCommand::SwcEdit::SwcPathLabeTraceMask::~SwcPathLabeTraceMask() {}

void ZStackDocCommand::SwcEdit::SwcPathLabeTraceMask::undo()
{
  m_branch.labelStack(m_doc->getTraceWorkspace()->trace_mask, 0);
}

void ZStackDocCommand::SwcEdit::SwcPathLabeTraceMask::redo()
{
  m_branch.labelStack(m_doc->getTraceWorkspace()->trace_mask, 255);
}

//////////////////

ZStackDocCommand::SwcEdit::SetRoot::SetRoot(
    ZStackDoc *doc, Swc_Tree_Node *tn, QUndoCommand *parent) :
  ZUndoCommand(parent), m_doc(doc), m_node(tn)
  //CompositeCommand(doc, parent)
{
  setText(QObject::tr("Set root"));
#if 0
  if (tn != NULL) {
    Swc_Tree_Node *buffer1, *buffer2, *buffer3;
    buffer1 = tn;
    buffer2 = buffer1->parent;
    new SetParent(doc, buffer1, NULL, this);
    //Swc_Tree_Node_Detach_Parent(buffer1);

    //weight[0] = buffer1->weight;
    while (Swc_Tree_Node_Is_Regular(buffer1) == TRUE) {
      if (Swc_Tree_Node_Is_Regular(buffer2) == TRUE) {
        //weight[1] = buffer2->weight;
        buffer3 = buffer2->parent;
        //buffer2->weight = weight[0];
        //weight[0] = weight[1];
        //Swc_Tree_Node_Add_Child(buffer1, buffer2);
        new SetParent(doc, buffer2, buffer1, this);
      }
      buffer1 = buffer2;
      buffer2 = buffer3;
    }

    //Swc_Tree_Node_Set_Parent(tn, buffer1);
    new SetParent(doc, tn, buffer1, this);
  }
#endif
}

void ZStackDocCommand::SwcEdit::SetRoot::redo()
{
  m_originalParentArray.clear();
  if (m_node != NULL) {
    Swc_Tree_Node *parent = SwcTreeNode::parent(m_node);

    while (SwcTreeNode::isRegular(parent)) {
      m_originalParentArray.push_back(parent);
      parent = SwcTreeNode::parent(parent);
    }
    Swc_Tree_Node *virtualRoot = parent;

    SwcTreeNode::setParent(m_node, virtualRoot);
    Swc_Tree_Node *currentParent = m_node;
    for (std::vector<Swc_Tree_Node*>::iterator
         iter = m_originalParentArray.begin();
         iter != m_originalParentArray.end(); ++iter) {
      SwcTreeNode::setParent(*iter, currentParent);
      currentParent = *iter;
    }

    m_doc->notifySwcModified();
  }
}

void ZStackDocCommand::SwcEdit::SetRoot::undo()
{
  if (!m_originalParentArray.empty()) {
    Swc_Tree_Node *virtualRoot = SwcTreeNode::parent(m_node);
    Swc_Tree_Node *currentParent = virtualRoot;
    for (std::vector<Swc_Tree_Node*>::reverse_iterator
         iter = m_originalParentArray.rbegin();
         iter != m_originalParentArray.rend(); ++iter) {
      SwcTreeNode::setParent(*iter, currentParent);
      currentParent = *iter;
    }
    SwcTreeNode::setParent(m_node, m_originalParentArray.front());

    m_doc->notifySwcModified();
  }
}


ZStackDocCommand::SwcEdit::ConnectSwcNode::ConnectSwcNode(
    ZStackDoc *doc, QUndoCommand *parent) :
  CompositeCommand(doc, parent)
{
  setText("Connect Swc Nodes");

  ZSwcConnector connector;
  std::set<Swc_Tree_Node*> nodeSet = doc->getSelectedSwcNodeSet();
  connector.setMinDist(SwcTreeNode::averageRadius(nodeSet.begin(),
                                                  nodeSet.end()) * 20.0);

  ZGraph *graph = connector.buildConnection(nodeSet);

  if (graph->size() > 0) {
    std::vector<Swc_Tree_Node*> nodeArray;
    for (set<Swc_Tree_Node*>::iterator iter = nodeSet.begin();
         iter != nodeSet.end(); ++iter) {
      nodeArray.push_back(*iter);
    }

    for (size_t i = 0; i < graph->size(); ++i) {
      if (graph->edgeWeight(i) > 0.0) {
        int e1 = graph->edgeStart(i);
        int e2 = graph->edgeEnd(i);
        TZ_ASSERT(e1 != e2, "Invalid edge");
        new SetRoot(doc, nodeArray[e2], this);
        new SetParent(doc, nodeArray[e2], nodeArray[e1], this);
      }
    }
    new RemoveEmptyTree(doc, this);
  }

  delete graph;
}

ZStackDocCommand::SwcEdit::RemoveSwc::RemoveSwc(
    ZStackDoc *doc, ZSwcTree *tree, QUndoCommand *parent) :
  ZUndoCommand(parent), m_doc(doc), m_tree(tree), m_role(ZDocPlayer::ROLE_NONE),
  m_isInDoc(true)
{
}

ZStackDocCommand::SwcEdit::RemoveSwc::~RemoveSwc()
{
#ifdef _DEBUG_
  std::cout << "ZStackDocCommand::SwcEdit::RemoveSwc destroyed" << std::endl;
#endif
  if (!m_isInDoc) {
    delete m_tree;
  }
}

void ZStackDocCommand::SwcEdit::RemoveSwc::redo()
{
  if (m_tree != NULL) {
    m_role = m_doc->removeObject(m_tree, false);
    m_isInDoc = false;
    m_doc->notifySwcModified();
  }
}

void ZStackDocCommand::SwcEdit::RemoveSwc::undo()
{
  if (m_tree != NULL) {
    //m_doc->addSwcTree(m_tree);
    m_doc->addObject(m_tree, NeuTube::Documentable_SWC, m_role);
    m_isInDoc = true;
  }
}

ZStackDocCommand::SwcEdit::RemoveEmptyTree::RemoveEmptyTree(
    ZStackDoc *doc, QUndoCommand *parent) : ZUndoCommand(parent), m_doc(doc)
{

}

ZStackDocCommand::SwcEdit::RemoveEmptyTree::~RemoveEmptyTree()
{
  for (std::set<ZSwcTree*>::iterator iter = m_emptyTreeSet.begin();
       iter != m_emptyTreeSet.end(); ++iter) {
    delete *iter;
  }
}

void ZStackDocCommand::SwcEdit::RemoveEmptyTree::redo()
{
  m_emptyTreeSet = m_doc->removeEmptySwcTree(false);
  if (!m_emptyTreeSet.empty()) {
    m_doc->notifySwcModified();
  }
}

void ZStackDocCommand::SwcEdit::RemoveEmptyTree::undo()
{
  for (std::set<ZSwcTree*>::iterator iter = m_emptyTreeSet.begin();
       iter != m_emptyTreeSet.end(); ++iter) {
    m_doc->addSwcTree(*iter);
  }
  m_emptyTreeSet.clear();
}

ZStackDocCommand::SwcEdit::BreakForest::BreakForest(
    ZStackDoc *doc, QUndoCommand *parent) :
  CompositeCommand(doc, parent)
{
  setText(QObject::tr("Break swc forest"));

  if (m_doc != NULL) {
    QList<ZSwcTree*> treeSet =
        m_doc->getSelectedObjectList<ZSwcTree>(ZStackObject::TYPE_SWC);
    //std::set<ZSwcTree*> *treeSet = m_doc->selectedSwcs();

    if (!treeSet.empty()) {
      for (QList<ZSwcTree*>::iterator iter = treeSet.begin();
           iter != treeSet.end(); ++iter) {
        Swc_Tree_Node *root = (*iter)->firstRegularRoot();
        if (root != NULL) {
          root = SwcTreeNode::nextSibling(root);
          while (root != NULL) {
            Swc_Tree_Node *sibling = SwcTreeNode::nextSibling(root);

            ZSwcTree *tree = new ZSwcTree;
            tree->forceVirtualRoot();
            new ZStackDocCommand::SwcEdit::SetParent(
                  doc, root, tree->root(), this);
            new ZStackDocCommand::SwcEdit::AddSwc(doc, tree, this);
            root = sibling;
          }
        }
      }
    }
  }
}

ZStackDocCommand::SwcEdit::GroupSwc::GroupSwc(
    ZStackDoc *doc, QUndoCommand *parent) : CompositeCommand(doc, parent)
{
  setText(QObject::tr("Group swc"));

  QList<ZSwcTree*> treeSet =
      m_doc->getSelectedObjectList<ZSwcTree>(ZStackObject::TYPE_SWC);
  //std::set<ZSwcTree*> *treeSet = m_doc->selectedSwcs();

  if (treeSet.size() > 1) {
    QList<ZSwcTree*>::iterator iter = treeSet.begin();
    Swc_Tree_Node *root = (*iter)->root();

    for (++iter; iter != treeSet.end(); ++iter) {
      Swc_Tree_Node *subroot = (*iter)->firstRegularRoot();
      new ZStackDocCommand::SwcEdit::SetParent(doc, subroot, root, this);
    }

    new ZStackDocCommand::SwcEdit::RemoveEmptyTree(doc, this);
  }
}

#if 0
ZStackDocCommand::SwcEdit::TraceSwcBranch::TraceSwcBranch(
    ZStackDoc *doc, double x, double y, double z, int c, QUndoCommand *parent) :
  QUndoCommand(parent), m_doc(doc), m_x(x), m_y(y), m_z(z), m_c(c)
{

}

ZStackDocCommand::SwcEdit::TraceSwcBranch::~TraceSwcBranch()
{

}

void ZStackDocCommand::SwcEdit::TraceSwcBranch::redo()
{
  ZNeuronTracer tracer;
  tracer.setIntensityField(m_doc->stack()->c_stack(m_c));
  tracer.setTraceWorkspace(m_doc->getTraceWorkspace());
  ZSwcPath branch = tracer.trace(m_x, m_y, m_z);
  tracer.updateMask(branch);
  ZSwcTree *tree = new ZSwcTree();
  tree->setDataFromNode(branch.front());
  m_doc->addSwcTree(tree, false);
  m_doc->notifySwcModified();
}

void ZStackDocCommand::SwcEdit::TraceSwcBranch::undo()
{

}
#endif

ZStackDocCommand::ObjectEdit::RemoveSelected::RemoveSelected(
    ZStackDoc *doc, QUndoCommand *parent) : QUndoCommand(parent), doc(doc)
{
  setText(QObject::tr("remove selected objects"));
}

ZStackDocCommand::ObjectEdit::RemoveSelected::~RemoveSelected()
{
#ifdef _DEBUG_
  std::cout << "RemoveSelected destroyed" << std::endl;
#endif

  for (QList<ZStackObject*>::iterator iter = m_selectedObject.begin();
       iter != m_selectedObject.end(); ++iter) {
    delete *iter;
  }
}

void ZStackDocCommand::ObjectEdit::RemoveSelected::undo()
{
  //Add the objects back
  doc->blockSignals(true);
  ZDocPlayer::TRole role = ZDocPlayer::ROLE_NONE;
  for (int i = 0; i < m_selectedObject.size(); ++i) {
    doc->addObject(m_selectedObject[i], m_roleList[i], false);
    role |= m_roleList[i];
  }
  doc->blockSignals(false);

  notifyObjectChanged(m_selectedObject, role);

  m_selectedObject.clear();
  m_roleList.clear();
}

void ZStackDocCommand::ObjectEdit::RemoveSelected::notifyObjectChanged(
    const QList<ZStackObject *> &selectedObject, ZDocPlayer::TRole role) const
{
  //Send notifications
  std::set<ZStackObject::EType> typeSet;
  for (QList<ZStackObject*>::const_iterator iter = selectedObject.begin();
       iter != selectedObject.end(); ++iter) {
    const ZStackObject *obj = *iter;
    typeSet.insert(obj->getType());
  }

  size_t s = typeSet.size();

  if (typeSet.count(ZStackObject::TYPE_PUNCTUM) > 0) {
    doc->notifyPunctumModified();
    --s;
  }

  if (typeSet.count(ZStackObject::TYPE_SWC) > 0) {
    doc->notifySwcModified();
    --s;
  }

  if (typeSet.count(ZStackObject::TYPE_STROKE) > 0) {
    doc->notifyStrokeModified();
    --s;
  }

  if (typeSet.count(ZStackObject::TYPE_SPARSE_OBJECT) > 0) {
    doc->notifySparseObjectModified();
    --s;
  }

  if (s > 0) {
    doc->notifyObjectModified();
  }

  doc->notifyPlayerChanged(role);
}

void ZStackDocCommand::ObjectEdit::RemoveSelected::redo()
{
  //Remove and backup selected objects
  m_selectedObject = doc->getObjectGroup().takeSelected();
  ZDocPlayer::TRole allRole = ZDocPlayer::ROLE_NONE;
  foreach (ZStackObject *obj, m_selectedObject) {
    ZDocPlayer::TRole role = doc->getPlayerList().removePlayer(obj);
    m_roleList.append(role);
    allRole |= role;
  }

  notifyObjectChanged(m_selectedObject, allRole);
}

ZStackDocCommand::TubeEdit::Trace::Trace(
    ZStackDoc *doc, int x, int y, int z, QUndoCommand *parent)
  : QUndoCommand(parent), m_doc(doc), m_x(x), m_y(y), m_z(z), m_c(0)
{
  setText(QObject::tr("trace tube from (%1,%2,%3)").arg(x).arg(y).arg(z));
}

ZStackDocCommand::TubeEdit::Trace::Trace(
    ZStackDoc *doc, int x, int y, int z, int c, QUndoCommand *parent)
  : QUndoCommand(parent), m_doc(doc), m_x(x), m_y(y), m_z(z), m_c(c)
{
  setText(QObject::tr("trace tube from (%1,%2,%3) in channel %4").
          arg(x).arg(y).arg(z).arg(c));
}

void ZStackDocCommand::TubeEdit::Trace::redo()
{
  if (m_doc->getStack()->depth() == 1) {
    m_chain = m_doc->traceRect(m_x, m_y, m_z, 3.0, m_c);
  } else {
    m_chain = m_doc->traceTube(m_x, m_y, m_z, 3.0, m_c);
  }
}

void ZStackDocCommand::TubeEdit::Trace::undo()
{
  if (m_chain != NULL) {
    m_doc->removeObject(m_chain, true);
    m_chain = NULL;
    m_doc->notifyChainModified();
  }
}
#if 0
ZStackDocCommand::TubeEdit::AutoTrace::AutoTrace(ZStackDoc *doc, QUndoCommand *parent)
  : QUndoCommand(parent), m_doc(doc)
{
  setText(QObject::tr("auto trace"));
}

ZStackDocCommand::TubeEdit::AutoTrace::~AutoTrace()
{
  for (int i=0; i<m_swcList.size(); i++) {
    delete m_swcList[i];
  }
  for (int i=0; i<m_obj3dList.size(); i++) {
    delete m_obj3dList[i];
  }
  for (int i=0; i<m_connList.size(); i++) {
    delete m_connList[i];
  }
  for (int i=0; i<m_punctaList.size(); i++) {
    delete m_punctaList[i];
  }
  for (int i=0; i<m_chainList.size(); i++) {
    delete m_chainList[i];
  }
  m_swcList.clear();
  m_obj3dList.clear();
  m_connList.clear();
  m_punctaList.clear();
  m_chainList.clear();
}

void ZStackDocCommand::TubeEdit::AutoTrace::undo()
{
  const QList<ZLocsegChain*> &chainList = m_doc->getChainList();
  const QList<ZSwcTree*> &swcList = m_doc->getSwcList();
  const QList<ZLocsegChainConn*> &connList = m_doc->getConnList();
  const QList<ZObject3d*> &obj3dList = m_doc->getObj3dList();
  const QList<ZPunctum*> &punctaList = m_doc->getPunctaList();
  m_doc->removeAllObject(false);
  // copy stuff back
  QMutableListIterator<ZPunctum*> iter1(m_punctaList);
  while (iter1.hasNext()) {
    ZPunctum *obj = iter1.next();
    m_doc->addPunctum(obj);
  }
  QMutableListIterator<ZSwcTree*> iter2(m_swcList);
  while (iter2.hasNext()) {
    ZSwcTree *obj = iter2.next();
    m_doc->addSwcTree(obj);
  }
  QMutableListIterator<ZObject3d*> iter3(m_obj3dList);
  while (iter3.hasNext()) {
    ZObject3d *obj = iter3.next();
    m_doc->addObj3d(obj);
  }
  QMutableListIterator<ZLocsegChain*> iter4(m_chainList);
  while (iter4.hasNext()) {
    ZLocsegChain *obj = iter4.next();
    m_doc->addLocsegChain(obj);
  }
  QMutableListIterator<ZLocsegChainConn*> iter(m_connList);
  while (iter.hasNext()) {
    ZLocsegChainConn *obj = iter.next();
    m_doc->addLocsegChainConn(obj);
  }
  if (!m_punctaList.empty() || !punctaList.empty()) {
    m_doc->notifyPunctumModified();
  }
  if (!m_swcList.empty() || !swcList.empty()) {
    m_doc->notifySwcModified();
  }
  if (!m_obj3dList.empty() || !obj3dList.empty()) {
    m_doc->notifyObj3dModified();
  }
  m_doc->notifyChainModified();
  m_chainList = chainList;
  m_swcList = swcList;
  m_connList = connList;
  m_obj3dList = obj3dList;
  m_punctaList = punctaList;
}
#endif

#if 0
void ZStackDocCommand::TubeEdit::AutoTrace::redo()
{
  QList<ZLocsegChain*> chainList = m_doc->getChainList();
  QList<ZSwcTree*> swcList = m_doc->getSwcList();
  QList<ZLocsegChainConn*> connList = m_doc->getConnList();
  QList<ZObject3d*> obj3dList = m_doc->getObj3dList();
  QList<ZPunctum*> punctaList = m_doc->getPunctaList();
  m_doc->removeAllObject(false);
  if (m_punctaList.isEmpty() && m_swcList.isEmpty() && m_obj3dList.isEmpty()
      && m_chainList.isEmpty() && m_connList.isEmpty()) {  // first time execute, trace
    m_doc->autoTrace();
  } else { // copy stuff back
    QMutableListIterator<ZPunctum*> iter1(m_punctaList);
    while (iter1.hasNext()) {
      ZPunctum *obj = iter1.next();
      m_doc->addPunctum(obj);
    }
    QMutableListIterator<ZSwcTree*> iter2(m_swcList);
    while (iter2.hasNext()) {
      ZSwcTree *obj = iter2.next();
      m_doc->addSwcTree(obj);
    }
    QMutableListIterator<ZObject3d*> iter3(m_obj3dList);
    while (iter3.hasNext()) {
      ZObject3d *obj = iter3.next();
      m_doc->addObj3d(obj);
    }
    QMutableListIterator<ZLocsegChain*> iter4(m_chainList);
    while (iter4.hasNext()) {
      ZLocsegChain *obj = iter4.next();
      m_doc->addLocsegChain(obj);
    }
    QMutableListIterator<ZLocsegChainConn*> iter(m_connList);
    while (iter.hasNext()) {
      ZLocsegChainConn *obj = iter.next();
      m_doc->addLocsegChainConn(obj);
    }
  }
  if (!m_punctaList.empty() || !punctaList.empty()) {
    m_doc->notifyPunctumModified();
  }
  if (!m_swcList.empty() || !swcList.empty()) {
    m_doc->notifySwcModified();
  }
  if (!m_obj3dList.empty() || !obj3dList.empty()) {
    m_doc->notifyObj3dModified();
  }
  m_doc->notifyChainModified();
  m_chainList = chainList;
  m_swcList = swcList;
  m_connList = connList;
  m_obj3dList = obj3dList;
  m_punctaList = punctaList;
}
#endif
ZStackDocCommand::TubeEdit::AutoTraceAxon::AutoTraceAxon(
    ZStackDoc *doc, QUndoCommand *parent)
  :QUndoCommand(parent), m_doc(doc)
{
  setText(QObject::tr("auto trace axon"));
}

ZStackDocCommand::TubeEdit::AutoTraceAxon::~AutoTraceAxon()
{
  for (int i=0; i<m_swcList.size(); i++) {
    delete m_swcList[i];
  }
  for (int i=0; i<m_obj3dList.size(); i++) {
    delete m_obj3dList[i];
  }
  for (int i=0; i<m_connList.size(); i++) {
    delete m_connList[i];
  }
  for (int i=0; i<m_punctaList.size(); i++) {
    delete m_punctaList[i];
  }
  for (int i=0; i<m_chainList.size(); i++) {
    delete m_chainList[i];
  }
  m_swcList.clear();
  m_obj3dList.clear();
  m_connList.clear();
  m_punctaList.clear();
  m_chainList.clear();
}

void ZStackDocCommand::TubeEdit::AutoTraceAxon::undo()
{
#if 0
  QList<ZLocsegChain*> chainList = m_doc->getChainList();
  QList<ZSwcTree*> swcList = m_doc->getSwcList();
  QList<ZLocsegChainConn*> connList = m_doc->getConnList();
  QList<ZObject3d*> obj3dList = m_doc->getObj3dList();
  QList<ZPunctum*> punctaList = m_doc->getPunctaList();
  m_doc->removeAllObject(false);
  // copy stuff back
  QMutableListIterator<ZPunctum*> iter1(m_punctaList);
  while (iter1.hasNext()) {
    ZPunctum *obj = iter1.next();
    m_doc->addPunctum(obj);
  }
  QMutableListIterator<ZSwcTree*> iter2(m_swcList);
  while (iter2.hasNext()) {
    ZSwcTree *obj = iter2.next();
    m_doc->addSwcTree(obj);
  }
  QMutableListIterator<ZObject3d*> iter3(m_obj3dList);
  while (iter3.hasNext()) {
    ZObject3d *obj = iter3.next();
    m_doc->addObj3d(obj);
  }
  QMutableListIterator<ZLocsegChain*> iter4(m_chainList);
  while (iter4.hasNext()) {
    ZLocsegChain *obj = iter4.next();
    m_doc->addLocsegChain(obj);
  }
  QMutableListIterator<ZLocsegChainConn*> iter(m_connList);
  while (iter.hasNext()) {
    ZLocsegChainConn *obj = iter.next();
    m_doc->addLocsegChainConn(obj);
  }
  if (!m_punctaList.empty() || !punctaList.empty()) {
    m_doc->notifyPunctumModified();
  }
  if (!m_swcList.empty() || !swcList.empty()) {
    m_doc->notifySwcModified();
  }
  if (!m_obj3dList.empty() || !obj3dList.empty()) {
    m_doc->notifyObj3dModified();
  }
  m_doc->notifyChainModified();
  m_chainList = chainList;
  m_swcList = swcList;
  m_connList = connList;
  m_obj3dList = obj3dList;
  m_punctaList = punctaList;
#endif
}

void ZStackDocCommand::TubeEdit::AutoTraceAxon::redo()
{
#if 0
  QList<ZLocsegChain*> chainList = m_doc->getChainList();
  QList<ZSwcTree*> swcList = m_doc->getSwcList();
  QList<ZLocsegChainConn*> connList = m_doc->getConnList();
  QList<ZObject3d*> obj3dList = m_doc->getObj3dList();
  QList<ZPunctum*> punctaList = m_doc->getPunctaList();
  m_doc->removeAllObject(false);
  if (m_punctaList.isEmpty() && m_swcList.isEmpty() && m_obj3dList.isEmpty()
      && m_chainList.isEmpty() && m_connList.isEmpty()) {  // first time execute, trace
    m_doc->autoTraceAxon();
  } else { // copy stuff back
    QMutableListIterator<ZPunctum*> iter1(m_punctaList);
    while (iter1.hasNext()) {
      ZPunctum *obj = iter1.next();
      m_doc->addPunctum(obj);
    }
    QMutableListIterator<ZSwcTree*> iter2(m_swcList);
    while (iter2.hasNext()) {
      ZSwcTree *obj = iter2.next();
      m_doc->addSwcTree(obj);
    }
    QMutableListIterator<ZObject3d*> iter3(m_obj3dList);
    while (iter3.hasNext()) {
      ZObject3d *obj = iter3.next();
      m_doc->addObj3d(obj);
    }
    QMutableListIterator<ZLocsegChain*> iter4(m_chainList);
    while (iter4.hasNext()) {
      ZLocsegChain *obj = iter4.next();
      m_doc->addLocsegChain(obj);
    }
    QMutableListIterator<ZLocsegChainConn*> iter(m_connList);
    while (iter.hasNext()) {
      ZLocsegChainConn *obj = iter.next();
      m_doc->addLocsegChainConn(obj);
    }
  }
  if (!m_punctaList.empty() || !punctaList.empty()) {
    m_doc->notifyPunctumModified();
  }
  if (!m_swcList.empty() || !swcList.empty()) {
    m_doc->notifySwcModified();
  }
  if (!m_obj3dList.empty() || !obj3dList.empty()) {
    m_doc->notifyObj3dModified();
  }
  m_doc->notifyChainModified();
  m_chainList = chainList;
  m_swcList = swcList;
  m_connList = connList;
  m_obj3dList = obj3dList;
  m_punctaList = punctaList;
#endif
}

// class ZStackDocCutSelectedLocsegChainCommand
ZStackDocCommand::TubeEdit::CutSegment::CutSegment(ZStackDoc *doc, QUndoCommand *parent)
  : QUndoCommand(parent), m_doc(doc)
{
  setText(QObject::tr("cut selected tubes"));
}

ZStackDocCommand::TubeEdit::CutSegment::~CutSegment()
{
  for (int i = 0; i < m_oldChainList.size(); i++) {
    delete m_oldChainList.at(i);
  }
}

void ZStackDocCommand::TubeEdit::CutSegment::redo()
{
  QList<ZLocsegChain*> chainList = m_doc->getLocsegChainList();
  for (int i = 0; i < chainList.size(); i++) {
    if (chainList.at(i)->isSelected() == true) {
      m_oldChainList.append(chainList.at(i));
      m_doc->cutLocsegChain(chainList.at(i), &m_newChainList);
    }
  }
  m_doc->getSelected(ZStackObject::TYPE_LOCSEG_CHAIN).clear();
}

void ZStackDocCommand::TubeEdit::CutSegment::undo()
{
  // restore previous selection state
  m_doc->deselectAllObject();

  for (int i = 0; i < m_newChainList.size(); i++) {
    m_doc->removeObject(m_newChainList.at(i), true);
  }
  m_newChainList.clear();
  for (int i = 0; i < m_oldChainList.size(); i++) {
    m_doc->addLocsegChain(m_oldChainList.at(i));
  }
  m_oldChainList.clear();
  m_doc->notifyChainModified();
}


// class ZStackDocBreakSelectedLocsegChainCommand
ZStackDocCommand::TubeEdit::BreakChain::BreakChain(ZStackDoc *doc, QUndoCommand *parent)
  : QUndoCommand(parent), m_doc(doc)
{
  setText(QObject::tr("break selected tubes"));
}

ZStackDocCommand::TubeEdit::BreakChain::~BreakChain()
{
  for (int i = 0; i < m_oldChainList.size(); i++) {
    delete m_oldChainList.at(i);
  }
}

void ZStackDocCommand::TubeEdit::BreakChain::redo()
{
  QList<ZLocsegChain*> chainList = m_doc->getLocsegChainList();
  for (int i = 0; i < chainList.size(); i++) {
    if (chainList.at(i)->isSelected() == true) {
      m_oldChainList.append(chainList.at(i));
      m_doc->breakLocsegChain(chainList.at(i), &m_newChainList);
    }
  }
  m_doc->getSelected(ZStackObject::TYPE_LOCSEG_CHAIN).clear();
  //m_doc->selectedChains()->clear();
}

void ZStackDocCommand::TubeEdit::BreakChain::undo()
{
  // restore previous selection state
  m_doc->deselectAllObject();

  for (int i = 0; i < m_newChainList.size(); i++) {
    m_doc->removeObject(m_newChainList.at(i), true);
  }
  m_newChainList.clear();
  for (int i = 0; i < m_oldChainList.size(); i++) {
    m_doc->addLocsegChain(m_oldChainList.at(i));
  }
  m_oldChainList.clear();
  m_doc->notifyChainModified();
}

ZStackDocCommand::TubeEdit::RemoveSmall::RemoveSmall(
    ZStackDoc *doc, double thre, QUndoCommand *parent)
  :QUndoCommand(parent), m_doc(doc), m_thre(thre)
{
  setText(QObject::tr("remove small tube use thre %1").arg(thre));
}

ZStackDocCommand::TubeEdit::RemoveSmall::~RemoveSmall()
{
  for (int i=0; i<m_chainList.size(); i++) {
    delete m_chainList[i];
  }
}

void ZStackDocCommand::TubeEdit::RemoveSmall::undo()
{
  for (int i=0; i<m_chainList.size(); i++) {
    m_doc->addLocsegChain(m_chainList[i]);
  }
  if (!m_chainList.empty())
    m_doc->notifyChainModified();
  m_chainList.clear();
}

void ZStackDocCommand::TubeEdit::RemoveSmall::redo()
{
  QList<ZLocsegChain*> chainList = m_doc->getLocsegChainList();
  QMutableListIterator<ZLocsegChain*> chainIter(chainList);
  while (chainIter.hasNext()) {
    ZLocsegChain *chain = chainIter.next();
    if (chain->geoLength() < m_thre) {
      m_doc->removeObject(chain, false);
      m_chainList.push_back(chain);
    }
  }
  if (!m_chainList.empty())
    m_doc->notifyChainModified();
}

ZStackDocCommand::TubeEdit::RemoveSelected::RemoveSelected(
    ZStackDoc *doc, QUndoCommand *parent) : QUndoCommand(parent), m_doc(doc)
{
}

ZStackDocCommand::TubeEdit::RemoveSelected::~RemoveSelected()
{
  for (int i=0; i<m_chainList.size(); i++) {
    delete m_chainList[i];
  }
}

void ZStackDocCommand::TubeEdit::RemoveSelected::redo()
{
  QList<ZLocsegChain*> chainList = m_doc->getSelectedObjectList<ZLocsegChain>(
        ZStackObject::TYPE_LOCSEG_CHAIN);
  for (QList<ZLocsegChain*>::iterator iter = chainList.begin();
       iter != chainList.end(); ++iter) {
    m_doc->removeObject(*iter, false);
    m_chainList.push_back(*iter);
  }

  if (!m_chainList.empty()) {
    m_doc->notifyChainModified();
  }
}

void ZStackDocCommand::TubeEdit::RemoveSelected::undo()
{
  for (int i=0; i<m_chainList.size(); i++) {
    m_doc->addLocsegChain(m_chainList[i]);
  }
  if (!m_chainList.empty()) {
    m_doc->notifyChainModified();
  }
  m_chainList.clear();
}

ZStackDocCommand::ObjectEdit::MoveSelected::MoveSelected(
    ZStackDoc *doc, double x, double y, double z, QUndoCommand *parent)
  : QUndoCommand(parent), m_doc(doc), m_x(x), m_y(y), m_z(z), m_swcMoved(false),
    m_punctaMoved(false), m_swcScaleX(1.), m_swcScaleY(1.), m_swcScaleZ(1.),
    m_punctaScaleX(1.), m_punctaScaleY(1.), m_punctaScaleZ(1.)
{
  setText(QObject::tr("Move Selected"));
}

ZStackDocCommand::ObjectEdit::MoveSelected::~MoveSelected()
{
}

ZStackDocCommand::ObjectEdit::AddObject::AddObject(ZStackDoc *doc, ZStackObject *obj,
    ZDocPlayer::TRole role, QUndoCommand *parent)
  : ZUndoCommand(parent), m_doc(doc), m_obj(obj), m_role(role),
    m_isInDoc(false)
{
  setText(QObject::tr("Add Object"));
}

ZStackDocCommand::ObjectEdit::AddObject::~AddObject()
{
  if (!m_isInDoc) {
    delete m_obj;
  }
}

void ZStackDocCommand::ObjectEdit::AddObject::redo()
{
  m_doc->addObject(m_obj, m_role);
  m_doc->notifyObjectModified();
  /*
  if ((m_role & ZDocPlayer::ROLE_3DPAINT) > 0) {
    m_doc->notifyVolumeModified();
  }
  if ((m_role & ZDocPlayer::ROLE_3DGRAPH_DECORATOR) > 0) {
    m_doc->notify3DGraphModified();
  }
  */
  m_isInDoc = true;
}

void ZStackDocCommand::ObjectEdit::AddObject::undo()
{
  m_doc->removeObject(m_obj, false);
  /*
  m_doc->notifyObjectModified();
  if ((role & ZDocPlayer::ROLE_3DPAINT) > 0) {
    m_doc->notifyVolumeModified();
  }
  if ((m_role & ZDocPlayer::ROLE_3DGRAPH_DECORATOR)) {
    m_doc->notify3DGraphModified();
  }*/
  m_isInDoc = false;
}


void ZStackDocCommand::ObjectEdit::MoveSelected::setSwcCoordScale(double x, double y, double z)
{
  m_swcScaleX = x;
  m_swcScaleY = y;
  m_swcScaleZ = z;
}

void ZStackDocCommand::ObjectEdit::MoveSelected::setPunctaCoordScale(double x, double y, double z)
{
  m_punctaScaleX = x;
  m_punctaScaleY = y;
  m_punctaScaleZ = z;
}

bool ZStackDocCommand::ObjectEdit::MoveSelected::mergeWith(const QUndoCommand *other)
{
  if (other->id() != id())
    return false;

  const MoveSelected *oth = dynamic_cast<const MoveSelected *>(other);
  if (oth != NULL) {
    if (m_punctaList != oth->m_punctaList ||
        m_swcNodeList != oth->m_swcNodeList ||
        m_swcList != oth->m_swcList ||
        m_swcScaleX != oth->m_swcScaleX ||
        m_swcScaleY != oth->m_swcScaleY ||
        m_swcScaleZ != oth->m_swcScaleZ ||
        m_punctaScaleX != oth->m_punctaScaleX ||
        m_punctaScaleY != oth->m_punctaScaleY ||
        m_punctaScaleZ != oth->m_punctaScaleZ) {
      return false;
    }

    m_x += oth->m_x;
    m_y += oth->m_y;
    m_z += oth->m_z;

    return true;
  }

  return false;
}

void ZStackDocCommand::ObjectEdit::MoveSelected::undo()
{
  for (QList<ZPunctum*>::iterator it = m_punctaList.begin();
       it != m_punctaList.end(); ++it) {
    (*it)->translate(-m_x/m_punctaScaleX, -m_y/m_punctaScaleY, -m_z/m_punctaScaleZ);
  }
  for (QList<ZSwcTree*>::iterator it = m_swcList.begin();
       it != m_swcList.end(); ++it) {
    (*it)->translate(-m_x/m_swcScaleX, -m_y/m_swcScaleY, -m_z/m_swcScaleZ);
  }
  for (std::set<Swc_Tree_Node*>::iterator it = m_swcNodeList.begin();
       it != m_swcNodeList.end(); ++it) {
    Swc_Tree_Node* tn = *it;
    tn->node.x -= m_x/m_swcScaleX;
    tn->node.y -= m_y/m_swcScaleY;
    tn->node.z -= m_z/m_swcScaleZ;
  }

  // restore selection state
  m_doc->blockSignals(true);
  m_doc->deselectAllObject();
  m_doc->setPunctumSelected(m_punctaList.begin(), m_punctaList.end(), true);
  m_doc->setSwcSelected(m_swcList.begin(), m_swcList.end(), true);
  m_doc->setSwcTreeNodeSelected(m_swcNodeList.begin(), m_swcNodeList.end(), true);
  m_doc->blockSignals(false);

  if (m_swcMoved)
    m_doc->notifySwcModified();
  if (m_punctaMoved)
    m_doc->notifyPunctumModified();
  m_swcMoved = false;
  m_punctaMoved = false;
}

void ZStackDocCommand::ObjectEdit::MoveSelected::redo()
{
  m_swcMoved = false;
  m_punctaMoved = false;

  m_punctaList =
      m_doc->getSelectedObjectList<ZPunctum>(ZStackObject::TYPE_PUNCTUM);
  if (!m_punctaList.empty())
    m_punctaMoved = true;


  m_swcNodeList = m_doc->getSelectedSwcNodeSet();
  if (!m_swcNodeList.empty())
    m_swcMoved = true;

  m_swcList = m_doc->getSelectedObjectList<ZSwcTree>(ZStackObject::TYPE_SWC);
  //m_swcList = *(m_doc->selectedSwcs());
  if (!m_swcList.empty())
    m_swcMoved = true;

  for (QList<ZPunctum*>::iterator it = m_punctaList.begin();
       it != m_punctaList.end(); ++it) {
    (*it)->translate(m_x/m_punctaScaleX, m_y/m_punctaScaleY, m_z/m_punctaScaleZ);
  }
  for (QList<ZSwcTree*>::iterator it = m_swcList.begin();
       it != m_swcList.end(); ++it) {
    (*it)->translate(m_x/m_swcScaleX, m_y/m_swcScaleY, m_z/m_swcScaleZ);
  }
  for (std::set<Swc_Tree_Node*>::iterator it = m_swcNodeList.begin();
       it != m_swcNodeList.end(); ++it) {
    Swc_Tree_Node* tn = *it;
    tn->node.x += m_x/m_swcScaleX;
    tn->node.y += m_y/m_swcScaleY;
    tn->node.z += m_z/m_swcScaleZ;
  }

  if (m_swcMoved)
    m_doc->notifySwcModified();
  if (m_punctaMoved)
    m_doc->notifyPunctumModified();
}

ZStackDocCommand::StrokeEdit::AddStroke::AddStroke(
    ZStackDoc *doc, ZStroke2d *stroke, QUndoCommand *parent) :
  QUndoCommand(parent), m_doc(doc), m_stroke(stroke), m_isInDoc(false)
{
  setText(QObject::tr("Add stroke"));
}

ZStackDocCommand::StrokeEdit::AddStroke::~AddStroke()
{
  if (!m_isInDoc) {
    delete m_stroke;
  }
}

void ZStackDocCommand::StrokeEdit::AddStroke::redo()
{
  m_doc->addStroke(m_stroke);
  m_isInDoc = true;
  m_doc->notifyStrokeModified();
}

void ZStackDocCommand::StrokeEdit::AddStroke::undo()
{
  m_doc->removeObject(m_stroke, false);
  m_isInDoc = false;
  m_doc->notifyStrokeModified();
}


ZStackDocCommand::StrokeEdit::RemoveTopStroke::RemoveTopStroke(
    ZStackDoc *doc, QUndoCommand *parent) :
  QUndoCommand(parent), m_doc(doc), m_stroke(NULL), m_isInDoc(true)
{
}

ZStackDocCommand::StrokeEdit::RemoveTopStroke::~RemoveTopStroke()
{
  if (!m_isInDoc) {
    delete m_stroke;
  }
}

void ZStackDocCommand::StrokeEdit::RemoveTopStroke::redo()
{
  m_stroke = m_doc->getStrokeList().front();
  if (m_stroke != NULL) {
    m_doc->removeObject(m_doc->getStrokeList().front(), false);
    m_isInDoc = false;
    m_doc->notifyStrokeModified();
  }
}

void ZStackDocCommand::StrokeEdit::RemoveTopStroke::undo()
{
  if (m_stroke != NULL) {
    m_doc->addStroke(m_stroke);
    m_doc->notifyStrokeModified();
    m_isInDoc = true;
  }
}

ZStackDocCommand::StrokeEdit::CompositeCommand::CompositeCommand(
    ZStackDoc *doc, QUndoCommand *parent) : QUndoCommand(parent), m_doc(doc)
{
}

ZStackDocCommand::StrokeEdit::CompositeCommand::~CompositeCommand()
{
  qDebug() << "Stroke composite command (" << this->text() << ") destroyed";
}

void ZStackDocCommand::StrokeEdit::CompositeCommand::redo()
{
  m_doc->blockSignals(true);
  QUndoCommand::redo();
  m_doc->blockSignals(false);
  m_doc->notifyStrokeModified();
}


void ZStackDocCommand::StrokeEdit::CompositeCommand::undo()
{
  m_doc->blockSignals(true);
  QUndoCommand::undo();
  m_doc->blockSignals(false);
  m_doc->notifyStrokeModified();
}

/////////////////////////////////////////////////////

ZStackDocCommand::StackProcess::Binarize::Binarize(
    ZStackDoc *doc, int thre, QUndoCommand *parent)
  :QUndoCommand(parent), doc(doc), zstack(NULL), thre(thre), success(false)
{
  setText(QObject::tr("Binarize Image with threshold %1").arg(thre));
}

ZStackDocCommand::StackProcess::Binarize::~Binarize()
{
  delete zstack;
}

void ZStackDocCommand::StackProcess::Binarize::undo()
{
  if (success) {
    doc->loadStack(zstack);
    //doc->requestRedrawStack();
    zstack = NULL;
    success = false;
  } else {
    delete zstack;
    zstack = NULL;
  }
}

void ZStackDocCommand::StackProcess::Binarize::redo()
{
  //zstack = new ZStack(*(doc->m_stack));
  zstack = doc->getStack()->clone();
  success = doc->binarize(thre);
}

ZStackDocCommand::StackProcess::BwSolid::BwSolid(
    ZStackDoc *doc, QUndoCommand *parent)
  :QUndoCommand(parent), doc(doc), zstack(NULL), success(false)
{
  setText(QObject::tr("Binary Image Solidify"));
}

ZStackDocCommand::StackProcess::BwSolid::~BwSolid()
{
  delete zstack;
}

void ZStackDocCommand::StackProcess::BwSolid::undo()
{
  if (success) {
    doc->loadStack(zstack);
    //doc->requestRedrawStack();
    zstack = NULL;
    success = false;
  } else {
    delete zstack;
    zstack = NULL;
  }
}

void ZStackDocCommand::StackProcess::BwSolid::redo()
{
  //zstack = new ZStack(*(doc->m_stack));
  zstack = doc->getStack()->clone();
  success = doc->bwsolid();
}

ZStackDocCommand::StackProcess::Watershed::Watershed(
    ZStackDoc *doc, QUndoCommand *parent)
  :QUndoCommand(parent), doc(doc), zstack(NULL), success(false)
{
  setText(QObject::tr("watershed"));
}

ZStackDocCommand::StackProcess::Watershed::~Watershed()
{
  delete zstack;
}

void ZStackDocCommand::StackProcess::Watershed::undo()
{
  if (success) {
    doc->loadStack(zstack);
    //doc->requestRedrawStack();
    zstack = NULL;
    success = false;
  } else {
    delete zstack;
    zstack = NULL;
  }
}

void ZStackDocCommand::StackProcess::Watershed::redo()
{
  //zstack = new ZStack(*(doc->m_stack));
  zstack = doc->getStack()->clone();
  success = doc->watershed();
}

ZStackDocCommand::StackProcess::EnhanceLine::EnhanceLine(
    ZStackDoc *doc, QUndoCommand *parent)
  :QUndoCommand(parent), doc(doc), zstack(NULL), success(false)
{
  setText(QObject::tr("Enhance Line"));
}

ZStackDocCommand::StackProcess::EnhanceLine::~EnhanceLine()
{
  delete zstack;
}

void ZStackDocCommand::StackProcess::EnhanceLine::undo()
{
  if (success) {
    doc->loadStack(zstack);
    //doc->requestRedrawStack();
    zstack = NULL;
    success = false;
  } else {
    delete zstack;
    zstack = NULL;
  }
}

void ZStackDocCommand::StackProcess::EnhanceLine::redo()
{
  //zstack = new ZStack(*(doc->m_stack));
  zstack = doc->getStack()->clone();
  success = doc->enhanceLine();
}

