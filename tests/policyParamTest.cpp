#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "patterns/creation/PolicyCatalog.h"
#include "patterns/execution/DepthFirstIterator.h"
#include "patterns/structural/LeafWorkItem.h"
#include "patterns/structural/Tividy.h"
#include "server/DemoSeed.h"

namespace {

class PolicyTest : public ::testing::TestWithParam<std::string> {
protected:
    Tividy engine;

    PolicyTest() : engine("Acme") {}

    void SetUp() override {
        DemoSeed::company(engine);
        DemoSeed::templates(engine);
        engine.clearHistory();
    }

    std::string approverFor(const std::string& project, const std::string& item) const {
        std::vector<Participant*> people = engine.organisation().participants();
        for (std::size_t i = 0; i < people.size(); ++i) {
            if (!engine.organisation().isOwner(people[i]->name()) &&
                engine.canApprove(people[i]->name(), project, item)) {
                return people[i]->name();
            }
        }
        return engine.canApprove("Olivia", project, item) ? "Olivia" : "";
    }

    bool step(const std::string& project) {
        bool moved = false;
        std::vector<std::string> ids;
        DepthFirstIterator it(&engine.instance(project).root());
        for (it.first(); !it.isDone(); it.next()) {
            if (dynamic_cast<LeafWorkItem*>(it.currentItem()) != nullptr) ids.push_back(it.currentItem()->id());
        }
        for (std::size_t i = 0; i < ids.size(); ++i) {
            const std::string status = engine.instance(project).item(ids[i])->status();
            if (status == WorkItem::AVAILABLE) {
                engine.autoAssign(project, ids[i]);
                moved = true;
            } else if (status == WorkItem::ASSIGNED) {
                engine.start(project, ids[i]);
                engine.complete(project, ids[i]);
                moved = true;
            } else if (status == WorkItem::AWAITING_APPROVAL) {
                std::string signer = approverFor(project, ids[i]);
                if (!signer.empty()) {
                    engine.approveAs(project, ids[i], signer);
                    moved = true;
                }
            }
        }
        return moved;
    }
};

TEST_P(PolicyTest, CatalogMakesTheNamedFactory) {
    std::unique_ptr<WorkflowFactory> factory = PolicyCatalog::create(GetParam());
    ASSERT_NE(factory, nullptr);
    EXPECT_EQ(factory->name(), GetParam());
    EXPECT_TRUE(PolicyCatalog::exists(GetParam()));
}

TEST_P(PolicyTest, ProjectsUseThePolicyTheyWereStartedWith) {
    std::string project = engine.startProject("Feature Delivery", "Dark mode", GetParam());
    EXPECT_EQ(engine.policyOf(project), GetParam());
    EXPECT_EQ(engine.instance(project).state(), WorkflowInstance::RUNNING);
}

TEST_P(PolicyTest, FeatureDeliveryRunsAllTheWayToTheOwner) {
    std::string project = engine.startProject("Feature Delivery", "Dark mode", GetParam());
    int rounds = 0;
    while (engine.instance(project).state() == WorkflowInstance::RUNNING && rounds < 50) {
        ASSERT_TRUE(step(project)) << "Nothing could move in round " << rounds << " under " << GetParam();
        ++rounds;
    }
    EXPECT_EQ(engine.instance(project).state(), WorkflowInstance::AWAITING_SIGNOFF);
    EXPECT_DOUBLE_EQ(engine.progress(project), 1.0);
    ASSERT_EQ(engine.signOffsFor("Olivia").size(), 1u);
    engine.signOff(project);
    EXPECT_EQ(engine.instance(project).state(), WorkflowInstance::COMPLETED);
}

TEST_P(PolicyTest, EveryLoadIsReleasedWhenTheWorkIsDone) {
    std::string project = engine.startProject("Feature Delivery", "Dark mode", GetParam());
    int rounds = 0;
    while (engine.instance(project).state() == WorkflowInstance::RUNNING && rounds < 50) {
        ASSERT_TRUE(step(project));
        ++rounds;
    }
    std::vector<Participant*> people = engine.organisation().participants();
    for (std::size_t i = 0; i < people.size(); ++i) {
        EXPECT_EQ(people[i]->load(), 0) << people[i]->name() << " still has work under " << GetParam();
    }
}

INSTANTIATE_TEST_SUITE_P(AllPolicies, PolicyTest,
                         ::testing::Values("SmallTeam", "BusyTeam", "HighRisk"));

}
