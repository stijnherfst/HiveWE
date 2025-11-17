import argparse
import json
import re

def parse_abilities(input_file, output_file):
    with open(input_file, 'r', encoding='utf-8') as f:
        text = f.read()

    abilities = {}
    blocks = re.split(r'^\s*-(.+?):\s*$', text, flags=re.MULTILINE)
    
    for i in range(1, len(blocks), 2):
        name = blocks[i].strip()
        block = blocks[i+1]

        rawcode_match = re.search(r'RawCode:\s*(\S+)', block)
        if not rawcode_match:
            continue
        rawcode = rawcode_match.group(1)

        tags_line = re.search(r'Tags:\s*(.*)', block)
        if tags_line:
            tags = re.findall(r'\[(.*?)\]', tags_line.group(1))
            tags = [t.strip() for t in tags]
        else:
            tags = []

        notes_match = re.search(r'Notes:\s*(.*)', block)
        notes = notes_match.group(1).strip() if notes_match else ""

        version_match = re.search(r'Latest tested version:\s*(\S+)', block)
        version = version_match.group(1) if version_match else ""

        raw_text_lines = []
        for line in block.splitlines():
            if not (line.strip().startswith("Tags:") or 
                    line.strip().startswith("Notes:") or 
                    line.strip().startswith("RawCode:") or 
                    line.strip().startswith("Latest tested version:")):
                raw_text_lines.append(line.rstrip())
        raw_text = "\n".join(raw_text_lines).strip()

        abilities[rawcode] = {
            "name": name,
            "tags": tags,
            "notes": notes,
            "latest_tested_version": version,
            "raw_text": raw_text
        }

    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(abilities, f, indent=4, ensure_ascii=False)

# Used to generate the ability_insights.json from the Ability Insight document
# https://docs.google.com/document/d/1z17FTnhyfVL87tJgLmwWks3Low6TuQ0tjfKHXBELWpo/edit?tab=t.0
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert ability text file to JSON.")
    parser.add_argument("input_file", help="Path to input text file")
    parser.add_argument("output_file", help="Path to output JSON file")
    args = parser.parse_args()

    parse_abilities(args.input_file, args.output_file)
