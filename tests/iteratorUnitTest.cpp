#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "test_utils.h"

#include "core/WorkItem.h"
#include "patterns/structural/WorkNode.h"
#include "patterns/structural/CompositeStage.h"
#include "patterns/execution/WorkIterator.h"
#include "patterns/execution/DepthFirstIterator.h"
#include "patterns/execution/ReadyOnlyIterator.h"

// ============================================================================
// SCENARIO HELPER: a Software Feature Delivery work tree
//
//   delivery (Feature Delivery)
//   |-- build (Build)
//   |   |-- implement (Implement feature)
//   |   `-- test      (Write tests)
//   `-- release (Release)
//       |-- review    (Code review)
//       `-- deploy (Deploy)
//           `-- prod  (Deploy to production)
// ============================================================================
static WorkItem* addItem(CompositeStage& stage, const std::string& id,
                         const std::string& name, const std::string& role) {
    WorkItem* item = new WorkItem(id, name, role);
    stage.add(std::unique_ptr<WorkNode>(item));
    return item;
}

static CompositeStage* addStage(CompositeStage& parent, const std::string& id,
                                const std::string& name) {
    CompositeStage* stage = new CompositeStage(id, name);
    parent.add(std::unique_ptr<WorkNode>(stage));
    return stage;
}

static void buildDeliveryTree(CompositeStage& root) {
    CompositeStage* build = addStage(root, "build", "Build");
    addItem(*build, "implement", "Implement feature", "Developer");
    addItem(*build, "test", "Write tests", "QA");

    CompositeStage* release = addStage(root, "release", "Release");
    addItem(*release, "review", "Code review", "Reviewer");
    CompositeStage* deploy = addStage(*release, "deploy", "Deploy");
    addItem(*deploy, "prod", "Deploy to production", "ReleaseManager");
}

// Walks the iterator from first() to the end and collects the ids it visits.
static std::vector<std::string> walk(WorkIterator& it) {
    std::vector<std::string> ids;
    for (it.first(); !it.isDone(); it.next()) {
        ids.push_back(it.currentItem()->id());
    }
    return ids;
}

static std::string join(const std::vector<std::string>& ids) {
    std::string result;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) {
            result += ",";
        }
        result += ids[i];
    }
    return result;
}

static WorkItem* itemAt(CompositeStage& root, const std::string& id) {
    return static_cast<WorkItem*>(root.find(id));
}

// ============================================================================
// TEST: DepthFirstIterator
// ============================================================================
void test_DepthFirstIterator() {
    std::cout << "\n--- Testing DepthFirstIterator ---\n";

    CompositeStage root("delivery", "Feature Delivery");
    buildDeliveryTree(root);

    DepthFirstIterator it(&root);
    check(it.isDone() == false, "DepthFirstIterator: a new iterator is positioned on the root");
    check(it.currentItem() == &root, "DepthFirstIterator: currentItem() starts at the root");

    check(join(walk(it)) == "delivery,build,implement,test,release,review,deploy,prod",
          "DepthFirstIterator: visits nodes in pre-order, children in definition order");

    check(it.isDone() == true, "DepthFirstIterator: isDone() after the last node");
    check(it.currentItem() == nullptr, "DepthFirstIterator: currentItem() is nullptr when done");
    it.next();
    check(it.isDone() == true, "DepthFirstIterator: next() past the end is harmless");

    it.first();
    check(it.currentItem() == &root, "DepthFirstIterator: first() restarts the walk");
    check(walk(it).size() == 8, "DepthFirstIterator: a restarted walk visits every node again");

    DepthFirstIterator subtree(root.find("release"));
    check(join(walk(subtree)) == "release,review,deploy,prod",
          "DepthFirstIterator: can walk just a subtree");

    WorkItem single("solo", "Solo task", "Developer");
    DepthFirstIterator leaf(&single);
    check(join(walk(leaf)) == "solo", "DepthFirstIterator: a leaf on its own is a one-node walk");

    DepthFirstIterator empty(nullptr);
    check(empty.isDone() == true, "DepthFirstIterator: a null root is an empty walk");
    check(empty.currentItem() == nullptr, "DepthFirstIterator: a null root has no current item");
}

// ============================================================================
// TEST: WorkNode::createIterator (factory method)
// ============================================================================
void test_CreateIterator() {
    std::cout << "\n--- Testing WorkNode::createIterator ---\n";

    CompositeStage root("delivery", "Feature Delivery");
    buildDeliveryTree(root);

    std::unique_ptr<WorkIterator> it = root.createIterator();
    check(it != nullptr, "createIterator: returns an iterator");
    check(join(walk(*it)) == "delivery,build,implement,test,release,review,deploy,prod",
          "createIterator: the iterator walks the whole tree depth-first");

    std::unique_ptr<WorkIterator> fromStage = root.find("build")->createIterator();
    check(join(walk(*fromStage)) == "build,implement,test",
          "createIterator: an iterator from a stage covers only that stage");

    // Counting items through the abstract interface only.
    int items = 0;
    for (it->first(); !it->isDone(); it->next()) {
        if (it->currentItem()->childCount() == 0) {
            ++items;
        }
    }
    check(items == 4, "createIterator: callers can count leaves without knowing the tree layout");
}

// ============================================================================
// TEST: ReadyOnlyIterator
// ============================================================================
void test_ReadyOnlyIterator() {
    std::cout << "\n--- Testing ReadyOnlyIterator ---\n";

    CompositeStage root("delivery", "Feature Delivery");
    buildDeliveryTree(root);

    ReadyOnlyIterator it(&root);
    check(it.isDone() == true, "ReadyOnlyIterator: nothing is ready before work is released");
    check(it.currentItem() == nullptr, "ReadyOnlyIterator: no current item when nothing is ready");

    itemAt(root, "implement")->makeAvailable();
    itemAt(root, "test")->makeAvailable();
    itemAt(root, "prod")->makeAvailable();

    check(join(walk(it)) == "implement,test,prod",
          "ReadyOnlyIterator: yields only ready items, in depth-first order");

    it.first();
    check(it.currentItem() == root.find("implement"),
          "ReadyOnlyIterator: first() lands on the first ready item");

    itemAt(root, "implement")->assign("Alice");
    check(join(walk(it)) == "test,prod",
          "ReadyOnlyIterator: an item stops being yielded once it is assigned");

    itemAt(root, "test")->cancel();
    itemAt(root, "prod")->assign("Erin");
    check(walk(it).empty(), "ReadyOnlyIterator: cancelled and taken work is skipped");

    itemAt(root, "review")->makeAvailable();
    ReadyOnlyIterator release(root.find("release"));
    check(join(walk(release)) == "review", "ReadyOnlyIterator: can be limited to a subtree");

    ReadyOnlyIterator empty(nullptr);
    check(empty.isDone() == true, "ReadyOnlyIterator: a null root is an empty walk");

    // Both concrete iterators are interchangeable behind WorkIterator.
    std::vector<std::unique_ptr<WorkIterator> > iterators;
    iterators.push_back(std::unique_ptr<WorkIterator>(new DepthFirstIterator(&root)));
    iterators.push_back(std::unique_ptr<WorkIterator>(new ReadyOnlyIterator(&root)));
    check(walk(*iterators[0]).size() == 8 && walk(*iterators[1]).size() == 1,
          "ReadyOnlyIterator: used through WorkIterator like any other iterator");
}

// ============================================================================
// MAIN
// ============================================================================
void run_iterator_tests() {
    std::cout << "\n--- Running Iterator (work tree traversal) Tests ---\n";
    int passedBefore = testsPassed;
    int failedBefore = testsFailed;

    test_DepthFirstIterator();
    test_CreateIterator();
    test_ReadyOnlyIterator();

    std::cout << "Iterator Tests: " << (testsPassed - passedBefore) << " passed, "
              << (testsFailed - failedBefore) << " failed.\n";
}
