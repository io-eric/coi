// The inner region's anchor is created inside the outer region's creation
// code; when inner flips true its content must sit between first and last.

export async function run({ page, expect }) {
  await page.waitForSelector(".late");
  const order = await page.evaluate(() =>
    Array.from(document.querySelectorAll(".root .row")).map((e) => e.className.replace("row ", "")).join(",")
  );
  if (order !== "first,inner,late,last") throw new Error(`wrong order: ${order}`);
}
