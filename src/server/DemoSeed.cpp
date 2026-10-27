#include "server/DemoSeed.h"
#include "patterns/structural/Tividy.h"
#include "patterns/creation/ConcreteDefinitionBuilder.h"
#include "patterns/creation/FeatureDeliveryDirector.h"

void DemoSeed::company(Tividy& engine) {
    engine.addUnit("", "engineering", "Engineering");
    engine.addUnit("engineering", "backend", "Back-end");
    engine.addUnit("engineering", "frontend", "Front-end");
    engine.addUnit("engineering", "qa", "QA");
    engine.addUnit("", "platform", "Platform");

    engine.addPerson("Olivia", "Owner", "org");
    engine.addPerson("Maya", "Manager", "engineering");
    engine.addPerson("Tom", "TechLead", "engineering");
    engine.addPerson("Rita", "Reviewer", "engineering");
    engine.addPerson("Bob", "Developer", "backend");
    engine.addPerson("Ben", "Developer", "backend");
    engine.addPerson("Fay", "Developer", "frontend");
    engine.addPerson("Dee", "Designer", "frontend");
    engine.addPerson("Quinn", "QA", "qa");
    engine.addPerson("Nina", "Manager", "platform");
    engine.addPerson("Ray", "ReleaseManager", "platform");
}

void DemoSeed::templates(Tividy& engine) {
    ConcreteDefinitionBuilder feature;
    FeatureDeliveryDirector director(&feature);
    director.construct("SmallTeam");
    engine.loadDefinition("Feature Delivery", feature.build());

    ConcreteDefinitionBuilder blank;
    blank.reset("Blank board");
    blank.setPolicy("BusyTeam");
    blank.setSignOff(false);
    blank.addStructuredStage(StageTemplate("board", "Board"));
    blank.addStructuredItem("board", ItemTemplate("kickoff", "Kick-off meeting", "Manager", "", "Manager"));
    engine.loadDefinition("Blank board", blank.build());
}
