#pragma once
#include <JuceHeader.h>

namespace PresetBrowser {
inline juce::String sound(const juce::var& patch) {
 auto category = patch["category"].toString();
 if (category == "Claude Compatibility")
  if (const auto* tags = patch["tags"].getArray(); tags && !tags->isEmpty())
   category = tags->getLast().toString();

 if (category.containsIgnoreCase("Bass")) return "Bass";
 if (category.containsIgnoreCase("Lead") || category.containsIgnoreCase("Solo")) return "Leads";
 if (category.containsIgnoreCase("Pluck")) return "Plucks";
 if (category.containsIgnoreCase("Key") || category.containsIgnoreCase("Bell")) return "Keys & Bells";
 if (category.containsIgnoreCase("Arp") || category.containsIgnoreCase("Sequence")) return "Arps & Sequences";
 if (category.containsIgnoreCase("Stab") || category.containsIgnoreCase("Chord")
     || category.containsIgnoreCase("Brass")) return "Chords & Stabs";
 if (category.containsIgnoreCase("Pad") || category.containsIgnoreCase("Atmosphere")) return "Pads";
 if (category.containsIgnoreCase("Vocal") || category.containsIgnoreCase("Formant")) return "Vocal & Formant";
 if (category.containsIgnoreCase("Texture") || category.containsIgnoreCase("Movement")) return "Textures";
 if (category.startsWithIgnoreCase("FX")) return "FX";
 return category == "Init" ? "Init" : "Other";
}

template <typename ReadPatch>
void categories(juce::ComboBox& box, int count, ReadPatch read) {
 juce::StringArray groups, banks;
 for (int i = 0; i < count; ++i) {
  const auto patch = read(i);
  groups.addIfNotAlreadyThere(sound(patch));
  if (patch["category"].toString() == "Claude Compatibility")
   banks.addIfNotAlreadyThere("Claude Compatibility");
  if (patch.hasProperty("claudeImport"))
   banks.addIfNotAlreadyThere("Claude Import");
  if (const auto* tags = patch["tags"].getArray(); tags && tags->contains("Signal Studies"))
   banks.addIfNotAlreadyThere("Signal Studies");
 }
 groups.sort(true);
 box.addItem("All sounds", 1);
 for (const auto& group : groups) box.addItem(group, box.getNumItems() + 1);
 if (!banks.isEmpty()) box.addSeparator();
 for (const auto& bank : banks) box.addItem(bank, box.getNumItems() + 1);
 box.setSelectedId(1, juce::dontSendNotification);
}

template <typename ReadPatch>
void rebuild(juce::ComboBox& box, const juce::ComboBox& category, const juce::String& search,
             int count, ReadPatch read) {
 box.clear(juce::dontSendNotification);
 juce::StringArray groups;
 juce::Array<juce::PopupMenu> menus;
 const auto filter = category.getText();
 const bool all = category.getSelectedId() <= 1;
 const bool bankFilter = filter == "Claude Compatibility" || filter == "Claude Import" || filter == "Signal Studies";
 for (int i = 0; i < count; ++i) {
  const auto patch = read(i);
  const auto group = sound(patch);
  const auto title = patch["name"].toString();
  const auto tags = patch["tags"];
  const bool inBank = filter == "Claude Compatibility" ? patch["category"].toString() == filter
                    : filter == "Claude Import" ? patch.hasProperty("claudeImport")
                    : tags.getArray() && tags.getArray()->contains("Signal Studies");
  if (!all && (bankFilter ? !inBank : group != filter)) continue;
  const auto query = search.trim();
  if (!(title + " " + group + " " + juce::JSON::toString(tags)).containsIgnoreCase(query)) continue;
  if (!all && !bankFilter) {
   box.addItem(title, i + 1);
   continue;
  }
  int index = groups.indexOf(group);
  if (index < 0) {
   index = groups.size();
   groups.add(group);
   menus.add(juce::PopupMenu{});
  }
  menus.getReference(index).addItem(i + 1, title);
 }
 if (all || bankFilter) {
  auto ordered = groups;
  ordered.sort(true);
  for (const auto& group : ordered) {
   const auto& menu = menus.getReference(groups.indexOf(group));
   box.getRootMenu()->addSubMenu(group + " (" + juce::String(menu.getNumItems()) + ")", menu);
  }
 }
}

inline int nextIndex(const juce::ComboBox& box, int current, int direction) {
 const int count = box.getNumItems();
 if (count == 0) return -1;
 for (int i = 0; i < count; ++i)
  if (box.getItemId(i) == current + 1)
   return box.getItemId((i + direction + count) % count) - 1;
 return box.getItemId(direction > 0 ? 0 : count - 1) - 1;
}
} // namespace PresetBrowser
