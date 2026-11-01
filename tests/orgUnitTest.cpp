#include <gtest/gtest.h>
#include "patterns/execution/AssignmentPipeline.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/Company.h"
#include "core/WorkItem.h"
#include "patterns/structural/OrgUnit.h"
#include "patterns/structural/OrgMember.h"
#include "patterns/execution/LeastLoadedAssignment.h"
#include "patterns/execution/RoleBasedAssignment.h"

namespace {

template <typename F>
bool throwsInvalid(F f) {
    try { f(); } catch (const std::invalid_argument&) { return true; } catch (...) { return false; }
    return false;
}

void seed(Company& org) {
    org.addUnit("", "engineering", "Engineering");
    org.addUnit("engineering", "backend", "Back-end");
    org.addUnit("engineering", "frontend", "Front-end");
    org.addParticipant("Maya", "Manager");
    org.addParticipant("Bob", "Developer");
    org.addParticipant("Ben", "Developer");
    org.addParticipant("Fay", "Developer");
    org.placeMember("engineering", "Maya");
    org.placeMember("backend", "Bob");
    org.placeMember("backend", "Ben");
    org.placeMember("frontend", "Fay");
}

TEST(CompanyComposite, Tree) {
    Company org("Acme");
    seed(org);

    OrgUnit& root = org.structure();
    EXPECT_TRUE(root.isUnit() && root.childCount() == 1) << "Org: the root unit holds the top-level team";
    OrgUnit* engineering = org.findUnit("engineering");
    EXPECT_TRUE(engineering != nullptr && engineering->subUnits().size() == 2) << "Org: Engineering has two sub-teams";

    std::vector<Participant*> all;
    engineering->collectMembers(all);
    EXPECT_EQ(all.size(), 4u) << "Org: collecting a composite gathers members of every sub-team";

    std::vector<Participant*> backend;
    org.findUnit("backend")->collectMembers(backend);
    EXPECT_EQ(backend.size(), 2u) << "Org: a leaf team only gathers its own members";

    EXPECT_TRUE(org.findUnit("backend")->path() == "Engineering / Back-end") << "Org: path() names the unit from the top";
    EXPECT_EQ(org.unitOf("Bob"), org.findUnit("backend")) << "Org: unitOf() finds a member's team";
    EXPECT_TRUE(org.inUnit("Bob", "engineering")) << "Org: inUnit() is true for members of a sub-team";
    EXPECT_FALSE(org.inUnit("Fay", "backend")) << "Org: inUnit() is false for another team";
    EXPECT_EQ(org.findUnit("Bob"), nullptr) << "Org: findUnit() does not return members";
    EXPECT_TRUE(org.isManager("Maya") && !org.isManager("Bob")) << "Org: isManager() reads the Manager role";
}

TEST(CompanyComposite, Changes) {
    Company org("Acme");
    seed(org);

    EXPECT_THROW(([&] { org.addUnit("", "backend", "Again"); })(), std::invalid_argument) << "Org: duplicate unit ids are rejected";
    EXPECT_THROW(([&] { org.addUnit("missing", "x", "X"); })(), std::invalid_argument) << "Org: an unknown parent is rejected";
    EXPECT_THROW(([&] { org.addUnit("", "Bob", "Bob team"); })(), std::invalid_argument) << "Org: a unit id cannot clash with a person";
    EXPECT_THROW(([&] { org.placeMember("backend", "Nobody"); })(), std::invalid_argument) << "Org: placing an unknown person is rejected";
    EXPECT_THROW(([&] { org.placeMember("nowhere", "Bob"); })(), std::invalid_argument) << "Org: placing into an unknown unit is rejected";

    org.placeMember("frontend", "Bob");
    EXPECT_EQ(org.unitOf("Bob"), org.findUnit("frontend")) << "Org: placeMember() moves a person";
    std::vector<Participant*> backend;
    org.findUnit("backend")->collectMembers(backend);
    EXPECT_EQ(backend.size(), 1u) << "Org: the old team loses the moved person";

    std::unique_ptr<OrgComponent> removed = org.findUnit("frontend")->remove("Fay");
    EXPECT_TRUE(removed && removed->parent() == nullptr) << "OrgUnit: remove() hands back the detached child";
    EXPECT_TRUE(!org.findUnit("frontend")->remove("Fay")) << "OrgUnit: removing a missing child returns nothing";
}

TEST(CompanyComposite, ScopedAssignment) {
    Company org("Acme");
    seed(org);

    EXPECT_EQ(org.eligible("Developer", "backend").size(), 2u) << "Org: eligible(role, unit) keeps to the unit";
    EXPECT_EQ(org.eligible("Developer", "engineering").size(), 3u) << "Org: eligible(role, unit) includes sub-teams";
    EXPECT_EQ(org.eligible("", "engineering").size(), 3u) << "Org: an empty role means anyone who is not a manager";
    EXPECT_EQ(org.eligible("Developer", "").size(), 3u) << "Org: an empty unit means the whole company";
    EXPECT_TRUE(org.eligible("Developer", "missing").empty()) << "Org: an unknown unit has nobody eligible";

    org.find("Bob")->addLoad(3);
    WorkItem item("fix", "Fix it", "Developer");
    item.setData("unit", "backend");
    LeastLoadedAssignment least;
    TeamFirstPipeline leastInTeam(org, least);
    EXPECT_EQ(leastInTeam.choose(item, "Developer").person->name(), "Ben") << "Bridge: LeastLoaded picks the least busy person in the team";

    WorkItem front("ui", "Build UI", "Developer");
    front.setData("unit", "frontend");
    RoleBasedAssignment roleBased;
    TeamFirstPipeline firstInTeam(org, roleBased);
    EXPECT_EQ(firstInTeam.choose(front, "Developer").person->name(), "Fay") << "Bridge: RoleBased only looks inside the item's team";

    WorkItem anyone("chore", "Tidy up", "");
    anyone.setData("unit", "engineering");
    Participant* chosen = leastInTeam.choose(anyone, "").person;
    EXPECT_TRUE(chosen != nullptr && chosen->name() != "Maya") << "Bridge: an empty role never picks a manager";

    org.find("Fay")->setAvailable(false);
    AssignmentPipeline::Result widened = firstInTeam.choose(front, "Developer");
    EXPECT_TRUE(widened.person != nullptr && widened.person->name() != "Fay") << "Bridge: nobody free in the team widens to the parent team";
    EXPECT_EQ(widened.via.find("widened"), 0u) << "Bridge: the result says it was widened";
    CompanyWidePipeline wide(org, roleBased);
    EXPECT_EQ(wide.choose(front, "Developer").via, "the whole company") << "Bridge: the same strategy behind a different pipeline";
}

}

