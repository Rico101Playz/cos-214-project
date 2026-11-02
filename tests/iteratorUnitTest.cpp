#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>


#include "composite/LeafTask.h"
#include "composite/Task.h"
#include "composite/CompositeTask.h"
#include "iterator/WorkIterator.h"
#include "iterator/DepthFirstIterator.h"
#include "iterator/FilterIterator.h"

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
static LeafTask* addItem(CompositeTask& stage, const std::string& id,
                         const std::string& name, const std::string& role) {
    LeafTask* item = new LeafTask(id, name, role);
    stage.add(std::unique_ptr<Task>(item));
    return item;
}

static CompositeTask* addStage(CompositeTask& parent, const std::string& id,
                                const std::string& name) {
    CompositeTask* stage = new CompositeTask(id, name);
    parent.add(std::unique_ptr<Task>(stage));
    return stage;
}

static void buildDeliveryTree(CompositeTask& root) {
    CompositeTask* build = addStage(root, "build", "Build");
    addItem(*build, "implement", "Implement feature", "Developer");
    addItem(*build, "test", "Write tests", "QA");

    CompositeTask* release = addStage(root, "release", "Release");
    addItem(*release, "review", "Code review", "Reviewer");
    CompositeTask* deploy = addStage(*release, "deploy", "Deploy");
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

static LeafTask* itemAt(CompositeTask& root, const std::string& id) {
    return static_cast<LeafTask*>(root.find(id));
}

// ============================================================================
// TEST: DepthFirstIterator
// ============================================================================
TEST(Iterator, DepthFirstIterator) {

    CompositeTask root("delivery", "Feature Delivery");
    buildDeliveryTree(root);

    DepthFirstIterator it(&root);
    EXPECT_FALSE(it.isDone()) << "DepthFirstIterator: a new iterator is positioned on the root";
    EXPECT_EQ(it.currentItem(), &root) << "DepthFirstIterator: currentItem() starts at the root";

    EXPECT_EQ(join(walk(it)), "delivery,build,implement,test,release,review,deploy,prod") << "DepthFirstIterator: visits nodes in pre-order, children in definition order";

    EXPECT_TRUE(it.isDone()) << "DepthFirstIterator: isDone() after the last node";
    EXPECT_EQ(it.currentItem(), nullptr) << "DepthFirstIterator: currentItem() is nullptr when done";
    it.next();
    EXPECT_TRUE(it.isDone()) << "DepthFirstIterator: next() past the end is harmless";

    it.first();
    EXPECT_EQ(it.currentItem(), &root) << "DepthFirstIterator: first() restarts the walk";
    EXPECT_EQ(walk(it).size(), 8u) << "DepthFirstIterator: a restarted walk visits every node again";

    DepthFirstIterator subtree(root.find("release"));
    EXPECT_EQ(join(walk(subtree)), "release,review,deploy,prod") << "DepthFirstIterator: can walk just a subtree";

    LeafTask single("solo", "Solo task", "Developer");
    DepthFirstIterator leaf(&single);
    EXPECT_EQ(join(walk(leaf)), "solo") << "DepthFirstIterator: a leaf on its own is a one-node walk";

    DepthFirstIterator empty(nullptr);
    EXPECT_TRUE(empty.isDone()) << "DepthFirstIterator: a null root is an empty walk";
    EXPECT_EQ(empty.currentItem(), nullptr) << "DepthFirstIterator: a null root has no current item";
}

// ============================================================================
// TEST: Task::createIterator (factory method)
// ============================================================================
TEST(Iterator, CreateIterator) {

    CompositeTask root("delivery", "Feature Delivery");
    buildDeliveryTree(root);

    std::unique_ptr<WorkIterator> it = root.createIterator();
    EXPECT_NE(it, nullptr) << "createIterator: returns an iterator";
    EXPECT_EQ(join(walk(*it)), "delivery,build,implement,test,release,review,deploy,prod") << "createIterator: the iterator walks the whole tree depth-first";

    std::unique_ptr<WorkIterator> fromStage = root.find("build")->createIterator();
    EXPECT_EQ(join(walk(*fromStage)), "build,implement,test") << "createIterator: an iterator from a stage covers only that stage";

    // Counting items through the abstract interface only.
    int items = 0;
    for (it->first(); !it->isDone(); it->next()) {
        if (it->currentItem()->childCount() == 0) {
            ++items;
        }
    }
    EXPECT_EQ(items, 4) << "createIterator: callers can count leaves without knowing the tree layout";
}

// ============================================================================
// TEST: FilterIterator
// ============================================================================
TEST(Iterator, FilterIterator) {

    CompositeTask root("delivery", "Feature Delivery");
    buildDeliveryTree(root);

    FilterIterator it(&root);
    EXPECT_TRUE(it.isDone()) << "FilterIterator: nothing is ready before work is released";
    EXPECT_EQ(it.currentItem(), nullptr) << "FilterIterator: no current item when nothing is ready";

    itemAt(root, "implement")->makeAvailable();
    itemAt(root, "test")->makeAvailable();
    itemAt(root, "prod")->makeAvailable();

    EXPECT_EQ(join(walk(it)), "implement,test,prod") << "FilterIterator: yields only ready items, in depth-first order";

    it.first();
    EXPECT_EQ(it.currentItem(), root.find("implement")) << "FilterIterator: first() lands on the first ready item";

    itemAt(root, "implement")->assign("Alice");
    EXPECT_EQ(join(walk(it)), "test,prod") << "FilterIterator: an item stops being yielded once it is assigned";

    itemAt(root, "test")->cancel();
    itemAt(root, "prod")->assign("Erin");
    EXPECT_TRUE(walk(it).empty()) << "FilterIterator: cancelled and taken work is skipped";

    itemAt(root, "review")->makeAvailable();
    FilterIterator release(root.find("release"));
    EXPECT_EQ(join(walk(release)), "review") << "FilterIterator: can be limited to a subtree";

    FilterIterator empty(nullptr);
    EXPECT_TRUE(empty.isDone()) << "FilterIterator: a null root is an empty walk";

    // Both concrete iterators are interchangeable behind WorkIterator.
    std::vector<std::unique_ptr<WorkIterator> > iterators;
    iterators.push_back(std::unique_ptr<WorkIterator>(new DepthFirstIterator(&root)));
    iterators.push_back(std::unique_ptr<WorkIterator>(new FilterIterator(&root)));
    EXPECT_TRUE(walk(*iterators[0]).size() == 8 && walk(*iterators[1]).size() == 1) << "FilterIterator: used through WorkIterator like any other iterator";
}

// ============================================================================
// MAIN
// ============================================================================

