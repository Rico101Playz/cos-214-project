#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>


#include "patterns/creation/WorkflowFactory.h"
#include "patterns/creation/SmallTeamFactory.h"
#include "patterns/creation/ConcreteDefinitionBuilder.h"
#include "patterns/creation/FeatureDeliveryProcess.h"
#include "patterns/creation/WorkflowAssembler.h"
#include "patterns/creation/HighRiskFactory.h"
#include "core/Organisation.h"
#include "patterns/creation/BusyTeamFactory.h"
#include "patterns/execution/ApprovalRequest.h"

// ============================================================================
// Helpers
// ============================================================================

// Runs a request for `role` through a freshly made chain; true when it ends
// up resolved (approved by somebody in the chain).
static bool chainResolves(const WorkflowFactory& factory, const std::string& role) {
    std::unique_ptr<ApprovalHandler> chain = factory.createApprovalChain();
    ApprovalRequest request(role);
    chain->handle(request);
    return request.isResolved();
}

// ============================================================================
// TEST: SmallTeamFactory
// ============================================================================
TEST(AbstractFactory, SmallTeamFactory) {

    SmallTeamFactory concrete;
    const WorkflowFactory& factory = concrete;   // used through the interface

    EXPECT_EQ(factory.name(), "SmallTeam") << "SmallTeam: knows its name";

    std::unique_ptr<CompositeStage> stage = factory.createStage("s1", "Build");
    EXPECT_NE(stage.get(), nullptr) << "SmallTeam: createStage() returns a stage";
    EXPECT_TRUE(stage->id() == "s1" && stage->name() == "Build") << "SmallTeam: the stage has the id and name it was given";
    EXPECT_TRUE(stage->mode().name() == "Sequential") << "SmallTeam: stages run Sequential";

    std::unique_ptr<LeafWorkItem> item = factory.createWorkItem("i1", "Implement login", "Developer");
    EXPECT_NE(item.get(), nullptr) << "SmallTeam: createWorkItem() returns a work item";
    EXPECT_TRUE(item->id() == "i1" && item->name() == "Implement login") << "SmallTeam: the item has the id and name it was given";
    EXPECT_TRUE(item->role() == "Developer") << "SmallTeam: the item has the role it was given";
    EXPECT_TRUE(item->dueHours() == 48) << "SmallTeam: work is due in 48 hours";
    EXPECT_TRUE(item->status() == WorkItem::CREATED) << "SmallTeam: a new item starts Created";

    std::unique_ptr<AssignmentStrategy> strategy = factory.createAssignmentStrategy();
    EXPECT_NE(strategy.get(), nullptr) << "SmallTeam: createAssignmentStrategy() returns a strategy";
    EXPECT_TRUE(strategy->name() == "RoleBased") << "SmallTeam: assigns by role only";

    EXPECT_TRUE(chainResolves(factory, "Auto")) << "SmallTeam: the chain approves 'Auto' automatically";
    EXPECT_TRUE(chainResolves(factory, "Reviewer")) << "SmallTeam: a Reviewer can approve";
    EXPECT_FALSE(chainResolves(factory, "TechLead")) << "SmallTeam: the chain has no Tech Lead step";
    EXPECT_FALSE(chainResolves(factory, "ReleaseManager")) << "SmallTeam: the chain has no Release Manager step";

    // Every call makes a new, independent object.
    std::unique_ptr<CompositeStage> other = factory.createStage("s2", "Test");
    EXPECT_NE(other.get(), stage.get()) << "SmallTeam: every createStage() call makes a new stage";
}

// ============================================================================
// TEST: BusyTeamFactory
// ============================================================================
TEST(AbstractFactory, BusyTeamFactory) {

    BusyTeamFactory concrete;
    const WorkflowFactory& factory = concrete;   // used through the interface

    EXPECT_EQ(factory.name(), "BusyTeam") << "BusyTeam: knows its name";

    std::unique_ptr<CompositeStage> stage = factory.createStage("s1", "Build");
    EXPECT_TRUE(stage->id() == "s1" && stage->name() == "Build") << "BusyTeam: the stage has the id and name it was given";
    EXPECT_TRUE(stage->mode().name() == "Parallel") << "BusyTeam: stages run Parallel";

    std::unique_ptr<LeafWorkItem> item = factory.createWorkItem("i1", "Fix bug", "Developer");
    EXPECT_TRUE(item->id() == "i1" && item->name() == "Fix bug") << "BusyTeam: the item has the id and name it was given";
    EXPECT_TRUE(item->role() == "Developer") << "BusyTeam: the item has the role it was given";
    EXPECT_TRUE(item->dueHours() == 24) << "BusyTeam: work is due in 24 hours";
    EXPECT_TRUE(item->status() == WorkItem::CREATED) << "BusyTeam: a new item starts Created";

    std::unique_ptr<AssignmentStrategy> strategy = factory.createAssignmentStrategy();
    EXPECT_TRUE(strategy->name() == "LeastLoaded") << "BusyTeam: assigns to the least loaded person";

    EXPECT_TRUE(chainResolves(factory, "Reviewer")) << "BusyTeam: a Reviewer can approve";
    EXPECT_TRUE(chainResolves(factory, "TechLead")) << "BusyTeam: the Tech Lead can approve";
    EXPECT_FALSE(chainResolves(factory, "Auto")) << "BusyTeam: nothing is approved automatically";
    EXPECT_FALSE(chainResolves(factory, "ReleaseManager")) << "BusyTeam: the chain has no Release Manager step";

    // The policies really differ from each other.
    SmallTeamFactory small;
    EXPECT_TRUE(small.createStage("x", "x")->mode().name() != stage->mode().name()) << "BusyTeam vs SmallTeam: the stages run differently";
    EXPECT_TRUE(small.createAssignmentStrategy()->name() != strategy->name()) << "BusyTeam vs SmallTeam: the assignment strategies differ";
}

// ============================================================================
// TEST: HighRiskFactory
// ============================================================================
TEST(AbstractFactory, HighRiskFactory) {

    HighRiskFactory concrete;
    const WorkflowFactory& factory = concrete;   // used through the interface

    EXPECT_EQ(factory.name(), "HighRisk") << "HighRisk: knows its name";

    std::unique_ptr<CompositeStage> stage = factory.createStage("s1", "Release");
    EXPECT_TRUE(stage->id() == "s1" && stage->name() == "Release") << "HighRisk: the stage has the id and name it was given";
    EXPECT_TRUE(stage->mode().name() == "Sequential") << "HighRisk: stages run Sequential";

    std::unique_ptr<LeafWorkItem> item = factory.createWorkItem("i1", "Deploy", "Reviewer");
    EXPECT_TRUE(item->id() == "i1" && item->name() == "Deploy") << "HighRisk: the item has the id and name it was given";
    EXPECT_TRUE(item->role() == "Reviewer") << "HighRisk: the item has the role it was given";
    EXPECT_TRUE(item->dueHours() == 8) << "HighRisk: work is due in 8 hours";
    EXPECT_TRUE(item->getData("risk") == "high") << "HighRisk: every item is tagged risk=high";

    std::unique_ptr<AssignmentStrategy> strategy = factory.createAssignmentStrategy();
    EXPECT_TRUE(strategy->name() == "RuleBased(LeastLoaded)") << "HighRisk: rule-based assignment that falls back to least loaded";

    // The parts fit together: the item's tag triggers the strategy's rule.
    Organisation org("Acme");
    org.addParticipant("Rita", "Reviewer");
    org.addParticipant("Tess", "TechLead");
    Participant* picked = strategy->assign(*item, "Reviewer", org);
    EXPECT_TRUE(picked != nullptr && picked->name() == "Tess") << "HighRisk: review of a risk=high item goes to the Tech Lead, not the Reviewer";

    // Nothing is approved automatically; every level can approve; timeouts escalate.
    EXPECT_FALSE(chainResolves(factory, "Auto")) << "HighRisk: nothing is approved automatically";
    EXPECT_TRUE(chainResolves(factory, "Reviewer")) << "HighRisk: a Reviewer can approve";
    EXPECT_TRUE(chainResolves(factory, "TechLead")) << "HighRisk: the Tech Lead can approve";
    EXPECT_TRUE(chainResolves(factory, "ReleaseManager")) << "HighRisk: the Release Manager can approve";
    EXPECT_TRUE(chainResolves(factory, "Timeout")) << "HighRisk: a timed-out request is escalated and approved";
    EXPECT_FALSE(chainResolves(factory, "CEO")) << "HighRisk: an unknown role is not approved";

    // The policies really differ from each other.
    BusyTeamFactory busy;
    EXPECT_TRUE(busy.createWorkItem("x", "x", "Reviewer")->hasData("risk") == false) << "HighRisk vs BusyTeam: only HighRisk tags its items";
    EXPECT_FALSE(chainResolves(busy, "ReleaseManager")) << "HighRisk vs BusyTeam: only HighRisk can reach the Release Manager";
}

// ============================================================================
// TEST: WorkflowAssembler (the client) with every factory
// ============================================================================
static int countItems(const WorkNode& node) {
    // Only real work items count; an empty stage is not an item.
    if (dynamic_cast<const LeafWorkItem*>(&node) != nullptr) return 1;
    int total = 0;
    for (std::size_t i = 0; i < node.childCount(); ++i) total += countItems(*node.child(i));
    return total;
}

TEST(AbstractFactory, WorkflowAssembler) {

    SmallTeamFactory small;
    BusyTeamFactory busy;
    HighRiskFactory risky;
    const WorkflowFactory* factories[] = { &small, &busy, &risky };
    const std::string modes[] = { "Sequential", "Parallel", "Sequential" };
    const int dueHours[] = { 48, 24, 8 };

    for (int f = 0; f < 3; ++f) {
        const WorkflowFactory& factory = *factories[f];
        const std::string who = factory.name() + ": ";

        std::unique_ptr<CompositeStage> tree = WorkflowAssembler::buildFeatureTree(factory);
        EXPECT_TRUE(tree->name() == "Feature Delivery") << who + "the tree is called Feature Delivery";
        EXPECT_TRUE(tree->childCount() == 3) << who + "three stages: Develop, Review, Release";
        EXPECT_TRUE(tree->child(0)->name() == "Develop" && tree->child(1)->name() == "Review" &&
              tree->child(2)->name() == "Release") << who + "the stages are in order";
        EXPECT_EQ(countItems(*tree), 4) << who + "four work items in total";

        // Every part in the tree belongs to the same policy.
        bool modesMatch = tree->mode().name() == modes[f];
        bool itemsMatch = true;
        for (std::size_t s = 0; s < tree->childCount(); ++s) {
            CompositeStage* stage = dynamic_cast<CompositeStage*>(tree->child(s));
            modesMatch = modesMatch && stage != nullptr && stage->mode().name() == modes[f];
            for (std::size_t i = 0; stage != nullptr && i < stage->childCount(); ++i) {
                LeafWorkItem* item = dynamic_cast<LeafWorkItem*>(stage->child(i));
                itemsMatch = itemsMatch && item != nullptr && item->dueHours() == dueHours[f];
            }
        }
        EXPECT_TRUE(modesMatch) << who + "every stage runs " + modes[f];
        EXPECT_TRUE(itemsMatch) << who + "every item is due in " + std::to_string(dueHours[f]) + " hours";

        // The roles on the items are what the assignment strategy works with.
        WorkNode* review = tree->child(1);
        LeafWorkItem* codeReview = dynamic_cast<LeafWorkItem*>(review->child(0));
        EXPECT_TRUE(codeReview != nullptr && codeReview->role() == "Reviewer") << who + "the code review item needs a Reviewer";

        Organisation org("Acme");
        org.addParticipant("Rita", "Reviewer");
        org.addParticipant("Tess", "TechLead");
        std::unique_ptr<AssignmentService> service = WorkflowAssembler::buildAssignmentService(factory, org);
        EXPECT_TRUE(service->strategy().name() == factory.createAssignmentStrategy()->name()) << who + "the service uses the policy's assignment strategy";

        Participant* picked = codeReview ? service->chooseAssignee(*codeReview, "Reviewer") : nullptr;
        const std::string expected = (f == 2) ? "Tess" : "Rita";
        EXPECT_TRUE(picked != nullptr && picked->name() == expected) << who + "code review goes to " + expected;
    }

    EXPECT_TRUE(WorkflowAssembler::buildFeatureTree(risky)->child(0)->childCount() == 2) << "The same client builds the same shape for any policy";
}

// ============================================================================
// TEST: Feature Delivery scenario (Builder + Abstract Factory + Strategy + Chain)
// ============================================================================
TEST(AbstractFactory, FeatureDeliveryScenario) {

    // 1. The Builder (already on dev) produces the Feature Delivery definition.
    ConcreteDefinitionBuilder builder;
    FeatureDeliveryProcess process;
    process.construct(&builder);
    WorkflowDefinition definition = builder.build();
    EXPECT_TRUE(!definition.stages.empty() && !definition.items.empty()) << "Scenario: the Builder's definition has stages and items";

    // 2. The same definition becomes a different workflow under each policy.
    SmallTeamFactory small;
    BusyTeamFactory busy;
    HighRiskFactory risky;
    std::unique_ptr<CompositeStage> smallTree = WorkflowAssembler::buildFromDefinition(small, definition, "Developer");
    std::unique_ptr<CompositeStage> busyTree  = WorkflowAssembler::buildFromDefinition(busy, definition, "Developer");
    std::unique_ptr<CompositeStage> riskyTree = WorkflowAssembler::buildFromDefinition(risky, definition, "Developer");

    EXPECT_TRUE(smallTree->name() == definition.name) << "Scenario: the root is named after the definition";
    EXPECT_TRUE(smallTree->childCount() == definition.stages.size()) << "Scenario: one stage per stage in the definition";
    EXPECT_TRUE(countItems(*smallTree) == static_cast<int>(definition.items.size())) << "Scenario: one work item per item in the definition";
    EXPECT_TRUE(countItems(*smallTree) == countItems(*busyTree) && countItems(*busyTree) == countItems(*riskyTree)) << "Scenario: the policies change how the work runs, not how much work there is";

    EXPECT_TRUE(smallTree->mode().name() == "Sequential" && busyTree->mode().name() == "Parallel") << "Scenario: small team runs Sequential, busy team runs Parallel";

    LeafWorkItem* smallItem = dynamic_cast<LeafWorkItem*>(smallTree->child(0)->child(0));
    LeafWorkItem* busyItem  = dynamic_cast<LeafWorkItem*>(busyTree->child(0)->child(0));
    LeafWorkItem* riskyItem = dynamic_cast<LeafWorkItem*>(riskyTree->child(0)->child(0));
    EXPECT_TRUE(smallItem && busyItem && riskyItem) << "Scenario: the first item in every tree is a work item";
    EXPECT_TRUE(smallItem->dueHours() == 48 && busyItem->dueHours() == 24 && riskyItem->dueHours() == 8) << "Scenario: deadlines tighten from small team to busy team to high risk";
    EXPECT_TRUE(!smallItem->hasData("risk") && !busyItem->hasData("risk") && riskyItem->getData("risk") == "high") << "Scenario: only the high-risk items carry the risk tag";

    // 3. The policy's assignment strategy and approval chain act on those items.
    Organisation org("Acme Software");
    org.addParticipant("Alice", "Developer");
    org.addParticipant("Bob", "Developer");
    org.find("Alice")->addLoad(2);   // Alice is busier than Bob

    std::unique_ptr<AssignmentService> smallService = WorkflowAssembler::buildAssignmentService(small, org);
    std::unique_ptr<AssignmentService> busyService  = WorkflowAssembler::buildAssignmentService(busy, org);
    Participant* bySmall = smallService->chooseAssignee(*smallItem, smallItem->role());
    Participant* byBusy  = busyService->chooseAssignee(*busyItem, busyItem->role());
    EXPECT_TRUE(bySmall != nullptr && bySmall->name() == "Alice") << "Scenario: small team hands work to the first developer, whatever their load";
    EXPECT_TRUE(byBusy != nullptr && byBusy->name() == "Bob") << "Scenario: busy team hands the same work to the lighter-loaded developer";

    EXPECT_TRUE(chainResolves(small, "Auto") && !chainResolves(busy, "Auto") && !chainResolves(risky, "Auto")) << "Scenario: only the small team lets a request be approved automatically";
    EXPECT_TRUE(!chainResolves(small, "ReleaseManager") && !chainResolves(busy, "ReleaseManager") &&
          chainResolves(risky, "ReleaseManager")) << "Scenario: only the high-risk policy reaches the Release Manager";
}

// ============================================================================
// MAIN
// ============================================================================

